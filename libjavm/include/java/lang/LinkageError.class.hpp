
#pragma once
#include <java/lang/Error.class.hpp>

namespace java::lang {

    class LinkageError final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(LinkageError) {

                JAVM_NATIVE_CLASS_NAME("java.lang.LinkageError")
                JAVM_NATIVE_CLASS_EXTENDS("java.lang.Error")

            }

    };

}