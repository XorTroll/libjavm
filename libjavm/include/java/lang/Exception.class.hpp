
#pragma once
#include <java/lang/Throwable.class.hpp>

namespace java::lang {

    class Exception : public Throwable {

        public:
            JAVM_NATIVE_CLASS_CTOR(Exception) {

                JAVM_NATIVE_CLASS_NAME("java.lang.Exception")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Throwable")

            }

    };

}