#pragma once
#include "./atom.h"
#include <string>
#include <sstream>
#include <openssl/sha.h>

namespace gtaf {

class AtomCanonical final : public Atom {
public:
    AtomCanonical(std::string type, std::string value)
        : m_type(std::move(type)), m_value(std::move(value)) {
        m_hash = compute_hash();
    }

    AtomType atom_type() const override {
        return AtomType::Canonical;
    }

    std::string type_name() const override {
        return m_type;
    }

    const std::string& value() const { return m_value; }
    const std::string& hash() const { return m_hash; }

private:
    std::string m_type;
    std::string m_value;
    std::string m_hash;

    std::string compute_hash() const {
        std::string input = m_type + ":" + m_value;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
               input.size(), hash);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            ss << std::hex << (int)hash[i];
        return ss.str();
    }
};

}
