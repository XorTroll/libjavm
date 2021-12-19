
#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <climits>

// TODO: enable logging by log types...?

#ifndef JAVM_LOG
#ifdef JAVM_DEBUG_LOG
#define JAVM_LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define JAVM_LOG(fmt, ...)
#endif
#endif

namespace javm {

    #define _JAVM_DEFINE_INT_TYPE(s) \
    using u##s = uint##s##_t; \
    using i##s = int##s##_t;

    _JAVM_DEFINE_INT_TYPE(8)
    _JAVM_DEFINE_INT_TYPE(16)
    _JAVM_DEFINE_INT_TYPE(32)
    _JAVM_DEFINE_INT_TYPE(64)

    #undef _JAVM_DEFINE_INT_TYPE

    template<typename N>
    inline constexpr N BE(N n) {
        auto be_n = n;
        
        auto n_ptr = reinterpret_cast<u8*>(&n);
        auto be_n_ptr = reinterpret_cast<u8*>(&be_n);
        for(size_t i = 0; i < sizeof(N); i++) {
            be_n_ptr[i] = n_ptr[sizeof(N) - (i + 1)];
        }

        return be_n;
    }

    // Using shared pointers to automatically dispose objects/etc properly

    template<typename T>
    using Ptr = std::shared_ptr<T>;

    // In Java, Strings are UTF-16

    using String = std::u16string;

    namespace ptr {

        template<typename T, typename ...Args>
        inline Ptr<T> New(Args &&...args) {
            return std::make_shared<T>(args...);
        }

        template<typename T>
        inline T GetValue(Ptr<T> ptr) {
            return *ptr.get();
        }

        template<typename T>
        inline void SetValue(Ptr<T> ptr, const T val) {
            *ptr.get() = val;
        }

        template<typename T>
        inline bool IsValid(Ptr<T> ptr) {
            return bool(ptr);
        }

        template<typename T>
        inline bool Equal(Ptr<T> ptr1, Ptr<T> ptr2) {
            return ptr1.get() == ptr2.get();
        }

        template<typename T>
        inline void Destroy(Ptr<T> &ptr_ref) {
            if(ptr_ref) {
                ptr_ref.reset();
                ptr_ref = nullptr;
            }
        }

        template<typename T, typename U>
        inline Ptr<T> CastTo(Ptr<U> ptr) {
            return std::dynamic_pointer_cast<T>(ptr);
        }

    }

}