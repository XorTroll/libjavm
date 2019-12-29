
#pragma once
#include <java/lang/Throwable.class.hpp>

namespace java::lang {

    class Exception : public Object {

        public:
            Exception() : Object("java.lang.Exception", "java.lang.Throwable") {
            }

            JAVM_NATIVE_CLASS_CTOR(Exception)
    };

}