
#pragma once
#include <java/lang/Exception.class.hpp>

namespace java::lang {

    class IllegalArgumentException final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(IllegalArgumentException) {

                JAVM_NATIVE_CLASS_NAME("java.lang.IllegalArgumentException")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Exception")

            }

    };

}