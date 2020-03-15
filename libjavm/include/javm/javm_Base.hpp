
#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <climits>

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
        N tmpn = n;
        u8 *nbuf = (u8*)&n;
        u8 *tmpbuf = (u8*)&tmpn;
        for(auto i = 0; i < sizeof(N); i++) {
            tmpbuf[i] = nbuf[sizeof(N) - (i + 1)];
        }
        return tmpn;
    }

    template<typename N>
    inline std::string ToHexString(N n) {
        std::stringstream strm;
        strm << std::hex << n;
        return strm.str();
    }

    template<typename T>
    struct ThisClassTypeDeduction {
        
        static_assert(!std::is_pointer_v<T> && std::is_reference_v<T>, "This must be done with '*this'!");
        typedef typename std::remove_reference<T>::type Class;
    };

    #define JAVM_THIS_CLASS_TYPE(this_ptr) javm::ThisClassTypeDeduction<decltype(*this_ptr)>::Class
    #define JAVM_CLASS_TYPE JAVM_THIS_CLASS_TYPE(this)

    /*
    
    This can be used to determine a class's type from a member function based on 'this'.

    Given class T, and doing this inside a member function of T,
    ClassTypeDeduction<decltype(*this)>::Class / JAVM_THIS_CLASS_TYPE(this) / JAVA_CLASS_TYPE would equal T
    
    */

    template<typename T>
    using Ptr = std::shared_ptr<T>;

    class PtrUtils {

        public:
            template<typename T, typename ...Args>
            static inline Ptr<T> New(Args &&...args) {
                return std::make_shared<T>(args...);
            }

            template<typename T>
            static inline T GetValue(Ptr<T> ptr) {
                return *ptr.get();
            }

            template<typename T>
            static inline void SetValue(Ptr<T> ptr, T val) {
                *ptr.get() = val;
            }

            template<typename T>
            static inline bool IsValid(Ptr<T> ptr) {
                return bool(ptr);
            }

            template<typename T>
            static inline bool Equal(Ptr<T> ptr1, Ptr<T> ptr2) {
                return ptr1.get() == ptr2.get();
            }

            template<typename T>
            static inline void Destroy(Ptr<T> &ptr_ref) {
                if(ptr_ref) {
                    ptr_ref.reset();
                    ptr_ref = nullptr;
                }
            }

            template<typename T, typename U>
            static inline Ptr<T> CastTo(Ptr<U> ptr) {
                return std::dynamic_pointer_cast<T>(ptr);
            }
    };

}