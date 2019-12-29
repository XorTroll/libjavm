
#pragma once
#include <java/lang/LinkageError.class.hpp>

namespace java::lang {

    class NoClassDefFoundError : public LinkageError {

        public:
            JAVM_NATIVE_CLASS_CTOR(NoClassDefFoundError) {

                JAVM_NATIVE_CLASS_NAME("java.lang.NoClassDefFoundError")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.LinkageError")

            }

    };

}