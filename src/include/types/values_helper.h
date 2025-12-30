#pragma once

#include "types.hpp"
#include <iostream>

namespace gtaf::types {

/**
 * @brief Helper to check the current type held by an AtomValue.
 */
struct ValueVisitor {
    void operator()(std::monostate) const { std::cout << "Null\n"; }
    void operator()(bool v) const         { std::cout << (v ? "true" : "false") << "\n"; }
    void operator()(int64_t v) const      { std::cout << v << "\n"; }
    void operator()(double v) const       { std::cout << v << "\n"; }
    void operator()(const std::string& v) const { std::cout << v << "\n"; }
    void operator()(const Vector& v) const { std::cout << "Vector[" << v.size() << "]\n"; }
    void operator()(const std::vector<uint8_t>& v) const { std::cout << "Blob[" << v.size() << "]\n"; }
};

/**
 * @brief Template helper for extracting data with a fallback.
 */
template<typename T>
T get_value_or(const AtomValue& val, T fallback) {
    if (auto* p = std::get_if<T>(&val)) {
        return *p;
    }
    return fallback;
}

} // namespace gtaf::types