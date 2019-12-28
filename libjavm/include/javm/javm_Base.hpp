
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace javm {

    #define _JAVM_DEFINE_INT_TYPE(s) \
    using u##s = uint##s##_t; \
    using i##s = int##s##_t;

    _JAVM_DEFINE_INT_TYPE(8)
    _JAVM_DEFINE_INT_TYPE(16)
    _JAVM_DEFINE_INT_TYPE(32)
    _JAVM_DEFINE_INT_TYPE(64)

    template<typename N>
    inline constexpr N BE(N n) {
        if constexpr(sizeof(N) == 2) {
            return __builtin_bswap16(n);
        }
        else if constexpr(sizeof(N) == 4) {
            return __builtin_bswap32(n);
        }
        else if constexpr(sizeof(N) == 8) {
            return __builtin_bswap64(n);
        }
        return n;
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
}