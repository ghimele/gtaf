#pragma once

#include "types.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

// Simple SHA-256-like hash implementation for content addressing
// In production, use a proper crypto library (OpenSSL, libsodium, etc.)

namespace gtaf::types {

namespace detail {
    // FNV-1a hash (64-bit) - simple but effective for demonstration
    // In production, replace with SHA-256
    inline uint64_t fnv1a_hash(const uint8_t* data, size_t len) {
        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr uint64_t FNV_PRIME = 1099511628211ULL;

        uint64_t hash = FNV_OFFSET_BASIS;
        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    // Hash accumulator for building composite hashes
    class HashAccumulator {
    public:
        void update(const void* data, size_t len) {
            const auto* bytes = static_cast<const uint8_t*>(data);
            m_buffer.insert(m_buffer.end(), bytes, bytes + len);
        }

        void update_string(const std::string& str) {
            update(str.data(), str.size());
        }

        uint64_t finalize() const {
            return fnv1a_hash(m_buffer.data(), m_buffer.size());
        }

    private:
        std::vector<uint8_t> m_buffer;
    };

} // namespace detail

/**
 * @brief Compute content-based hash for an AtomValue
 *
 * This creates a deterministic hash based on the type and value,
 * suitable for content-addressed storage and deduplication.
 *
 * @param type_tag The semantic type of the atom (e.g., "user.name")
 * @param value The atom value to hash
 * @return 128-bit hash as AtomId
 */
inline AtomId compute_content_hash(const std::string& type_tag, const AtomValue& value) {
    detail::HashAccumulator hasher;

    // Hash the type tag first
    hasher.update_string(type_tag);

    // Hash the variant index (to distinguish types)
    size_t variant_index = value.index();
    hasher.update(&variant_index, sizeof(variant_index));

    // Hash the value based on its type
    std::visit([&hasher](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            // Nothing to hash for null
        }
        else if constexpr (std::is_same_v<T, bool>) {
            uint8_t b = arg ? 1 : 0;
            hasher.update(&b, sizeof(b));
        }
        else if constexpr (std::is_same_v<T, int64_t>) {
            hasher.update(&arg, sizeof(arg));
        }
        else if constexpr (std::is_same_v<T, double>) {
            hasher.update(&arg, sizeof(arg));
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            hasher.update_string(arg);
        }
        else if constexpr (std::is_same_v<T, Vector>) {
            // Hash vector dimensions and values
            size_t size = arg.size();
            hasher.update(&size, sizeof(size));
            hasher.update(arg.data(), arg.size() * sizeof(float));
        }
        else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            // Hash blob size and content
            size_t size = arg.size();
            hasher.update(&size, sizeof(size));
            hasher.update(arg.data(), arg.size());
        }
        else if constexpr (std::is_same_v<T, EdgeValue>) {
            // Hash target entity and relation
            hasher.update(arg.target.bytes.data(), arg.target.bytes.size());
            hasher.update_string(arg.relation);
        }
    }, value);

    // Finalize to 64-bit hash, then extend to 128-bit
    uint64_t hash1 = hasher.finalize();

    // Create second hash by rehashing with a salt
    detail::HashAccumulator hasher2;
    hasher2.update(&hash1, sizeof(hash1));
    uint64_t salt = 0xDEADBEEFCAFEBABEULL;
    hasher2.update(&salt, sizeof(salt));
    uint64_t hash2 = hasher2.finalize();

    // Combine into 128-bit AtomId
    AtomId atom_id{};
    std::memcpy(atom_id.bytes.data(), &hash1, sizeof(hash1));
    std::memcpy(atom_id.bytes.data() + 8, &hash2, sizeof(hash2));

    return atom_id;
}

/**
 * @brief Convert AtomId to hex string for debugging
 */
inline std::string atom_id_to_hex(const AtomId& id) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < id.bytes.size(); ++i) {
        oss << std::setw(2) << static_cast<int>(id.bytes[i]);
    }
    return oss.str();
}

} // namespace gtaf::types
