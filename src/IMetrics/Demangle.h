#pragma once

#include <string>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#include <memory>
#endif

inline std::string demangle(const char* name) {
#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res{
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
#elif defined(_MSC_VER)
    return std::string(name);
#else
    return std::string(name);
#endif
}