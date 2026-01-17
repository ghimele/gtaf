#include "persistence.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace gtaf::core {

// ---- BinaryWriter Implementation ----

BinaryWriter::BinaryWriter(const std::string& filepath)
    : m_stream(filepath, std::ios::binary | std::ios::out | std::ios::trunc)
{
    if (!m_stream) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }
}

BinaryWriter::~BinaryWriter() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
}

void BinaryWriter::write_u8(uint8_t value) {
    m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryWriter::write_u32(uint32_t value) {
    m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryWriter::write_u64(uint64_t value) {
    m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BinaryWriter::write_bytes(const void* data, size_t size) {
    m_stream.write(reinterpret_cast<const char*>(data), size);
}

void BinaryWriter::write_string(const std::string& str) {
    write_u32(static_cast<uint32_t>(str.size()));
    if (!str.empty()) {
        write_bytes(str.data(), str.size());
    }
}

void BinaryWriter::write_atom_id(const types::AtomId& id) {
    write_bytes(id.bytes.data(), id.bytes.size());
}

void BinaryWriter::write_entity_id(const types::EntityId& id) {
    write_bytes(id.bytes.data(), id.bytes.size());
}

void BinaryWriter::write_atom_value(const types::AtomValue& value) {
    // Write variant index
    write_u8(static_cast<uint8_t>(value.index()));

    // Write value based on type
    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            // Nothing to write
        } else if constexpr (std::is_same_v<T, bool>) {
            write_u8(arg ? 1 : 0);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            write_u64(static_cast<uint64_t>(arg));
        } else if constexpr (std::is_same_v<T, double>) {
            write_bytes(&arg, sizeof(double));
        } else if constexpr (std::is_same_v<T, std::string>) {
            write_string(arg);
        } else if constexpr (std::is_same_v<T, std::vector<float>>) {
            write_u32(static_cast<uint32_t>(arg.size()));
            write_bytes(arg.data(), arg.size() * sizeof(float));
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            write_u32(static_cast<uint32_t>(arg.size()));
            write_bytes(arg.data(), arg.size());
        } else if constexpr (std::is_same_v<T, types::EdgeValue>) {
            write_entity_id(arg.target);
            write_string(arg.relation);
        }
    }, value);
}

void BinaryWriter::write_lsn(const types::LogSequenceNumber& lsn) {
    write_u64(lsn.value);
}

void BinaryWriter::write_timestamp(types::Timestamp ts) {
    write_u64(ts);
}

// ---- BinaryReader Implementation ----

BinaryReader::BinaryReader(const std::string& filepath)
    : m_stream(filepath, std::ios::binary | std::ios::in)
{
    if (!m_stream) {
        throw std::runtime_error("Failed to open file for reading: " + filepath);
    }
    // Pre-allocate buffer
    m_buffer.resize(BUFFER_SIZE);
    std::cerr << "[DEBUG] BinaryReader: Using " << (BUFFER_SIZE / (1024*1024)) << "MB buffer" << std::endl;
    // Fill initial buffer
    refill_buffer();
}

BinaryReader::~BinaryReader() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
}

void BinaryReader::refill_buffer() {
    // Move remaining data to start of buffer
    size_t remaining = m_buffer_end - m_buffer_pos;
    if (remaining > 0 && m_buffer_pos > 0) {
        std::memmove(m_buffer.data(), m_buffer.data() + m_buffer_pos, remaining);
    }
    m_buffer_pos = 0;
    m_buffer_end = remaining;

    // Read more data
    m_stream.read(m_buffer.data() + remaining, BUFFER_SIZE - remaining);
    m_buffer_end += m_stream.gcount();
}

void BinaryReader::ensure_available(size_t bytes) {
    if (m_buffer_pos + bytes > m_buffer_end) {
        refill_buffer();
    }
}

uint8_t BinaryReader::read_u8() {
    ensure_available(1);
    return static_cast<uint8_t>(m_buffer[m_buffer_pos++]);
}

uint32_t BinaryReader::read_u32() {
    ensure_available(4);
    uint32_t value;
    std::memcpy(&value, m_buffer.data() + m_buffer_pos, sizeof(value));
    m_buffer_pos += 4;
    return value;
}

uint64_t BinaryReader::read_u64() {
    ensure_available(8);
    uint64_t value;
    std::memcpy(&value, m_buffer.data() + m_buffer_pos, sizeof(value));
    m_buffer_pos += 8;
    return value;
}

void BinaryReader::read_bytes(void* data, size_t size) {
    char* dest = reinterpret_cast<char*>(data);
    while (size > 0) {
        ensure_available(1);
        size_t available = m_buffer_end - m_buffer_pos;
        size_t to_copy = std::min(size, available);
        std::memcpy(dest, m_buffer.data() + m_buffer_pos, to_copy);
        m_buffer_pos += to_copy;
        dest += to_copy;
        size -= to_copy;
    }
}

std::string BinaryReader::read_string() {
    uint32_t length = read_u32();
    if (length == 0) return "";

    std::string result(length, '\0');
    read_bytes(result.data(), length);
    return result;
}

types::AtomId BinaryReader::read_atom_id() {
    types::AtomId id;
    read_bytes(id.bytes.data(), id.bytes.size());
    return id;
}

types::EntityId BinaryReader::read_entity_id() {
    types::EntityId id;
    read_bytes(id.bytes.data(), id.bytes.size());
    return id;
}

types::AtomValue BinaryReader::read_atom_value() {
    uint8_t index = read_u8();

    switch (index) {
        case 0: // monostate
            return std::monostate{};
        case 1: { // bool
            return read_u8() != 0;
        }
        case 2: { // int64_t
            return static_cast<int64_t>(read_u64());
        }
        case 3: { // double
            double value;
            read_bytes(&value, sizeof(double));
            return value;
        }
        case 4: { // string
            return read_string();
        }
        case 5: { // vector<float>
            uint32_t size = read_u32();
            std::vector<float> vec(size);
            read_bytes(vec.data(), size * sizeof(float));
            return vec;
        }
        case 6: { // vector<uint8_t>
            uint32_t size = read_u32();
            std::vector<uint8_t> vec(size);
            read_bytes(vec.data(), size);
            return vec;
        }
        case 7: { // EdgeValue
            types::EdgeValue edge;
            edge.target = read_entity_id();
            edge.relation = read_string();
            return edge;
        }
        default:
            throw std::runtime_error("Unknown variant index in atom value");
    }
}

types::LogSequenceNumber BinaryReader::read_lsn() {
    return types::LogSequenceNumber{read_u64()};
}

types::Timestamp BinaryReader::read_timestamp() {
    return read_u64();
}

} // namespace gtaf::core
