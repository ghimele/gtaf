#pragma once

#include "../types/types.h"
#include "atom.h"
#include <fstream>
#include <vector>
#include <string>

namespace gtaf::core {

/**
 * @brief Binary serialization utilities for GTAF persistence
 *
 * Provides low-level primitives for writing/reading GTAF data structures
 * to/from binary streams. Format is designed for:
 * - Simple implementation
 * - Fast load times
 * - Forward compatibility via versioning
 */
class BinaryWriter {
public:
    explicit BinaryWriter(const std::string& filepath);
    ~BinaryWriter();

    // Primitive types
    void write_u8(uint8_t value);
    void write_u32(uint32_t value);
    void write_u64(uint64_t value);
    void write_bytes(const void* data, size_t size);

    // GTAF types
    void write_string(const std::string& str);
    void write_atom_id(const types::AtomId& id);
    void write_entity_id(const types::EntityId& id);
    void write_atom_value(const types::AtomValue& value);
    void write_lsn(const types::LogSequenceNumber& lsn);
    void write_timestamp(types::Timestamp ts);

    bool is_open() const { return m_stream.is_open(); }

private:
    std::ofstream m_stream;
};

/**
 * @brief Binary deserialization for GTAF data
 */
class BinaryReader {
public:
    explicit BinaryReader(const std::string& filepath);
    ~BinaryReader();

    // Primitive types
    uint8_t read_u8();
    uint32_t read_u32();
    uint64_t read_u64();
    void read_bytes(void* data, size_t size);

    // GTAF types
    std::string read_string();
    types::AtomId read_atom_id();
    types::EntityId read_entity_id();
    types::AtomValue read_atom_value();
    types::LogSequenceNumber read_lsn();
    types::Timestamp read_timestamp();

    bool is_open() const { return m_stream.is_open(); }
    bool eof() const { return m_stream.eof(); }

private:
    std::ifstream m_stream;
};

} // namespace gtaf::core
