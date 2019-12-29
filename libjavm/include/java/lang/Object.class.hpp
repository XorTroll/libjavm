
#pragma once
#include <javm/native/native_Class.hpp>

namespace java {

    // Let's simplify types
    using namespace javm;

    namespace lang {

        // Any new native class should inherit from Object!
        class Object : public native::Class {

            public:
                JAVM_NATIVE_CLASS_CTOR(Object) {

                    JAVM_NATIVE_CLASS_NAME("java.lang.Object")
                    // Implicitly extend nothing, since classes extend Object by default
                    JAVM_NATIVE_CLASS_EXTENDS("")
                    
                }

        };

    }

}