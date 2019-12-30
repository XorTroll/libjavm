
#pragma once
#include <java/lang/Exception.class.hpp>

namespace java::lang {

    class RuntimeException : public Exception {

        public:
            JAVM_NATIVE_CLASS_CTOR(RuntimeException) {

                JAVM_NATIVE_CLASS_NAME("java.lang.RuntimeException")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Exception")

            }

    };

}