
#pragma once
#include <java/lang/Throwable.class.hpp>

namespace java::lang {

    class Error final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(Error) {

                JAVM_NATIVE_CLASS_NAME("java.lang.Error")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Throwable")

            }

    };

}