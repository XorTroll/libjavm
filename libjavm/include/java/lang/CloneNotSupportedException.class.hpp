
#pragma once
#include <java/lang/Exception.class.hpp>

namespace java::lang {

    class CloneNotSupportedException final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(CloneNotSupportedException) {

                JAVM_NATIVE_CLASS_NAME("java.lang.CloneNotSupportedException")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Exception")

            }

    };

}