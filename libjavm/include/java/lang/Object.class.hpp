
#pragma once
#include <javm/native/native_Class.hpp>

namespace java {

    // Let's simplify types
    using namespace javm;

    namespace lang {

        // Any new native class should inherit from Object!
        class Object : public native::Class {

            public:
                Object(std::string name = "java.lang.Object") : Class(name) {}

                JAVM_NATIVE_CLASS_CTOR(Object)
        };

    }

}