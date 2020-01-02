
#pragma once
#include <java/io/PrintStream.class.hpp>

namespace java::lang {

    class System final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(System) {

                JAVM_NATIVE_CLASS_NAME("java.lang.System")

                JAVM_NATIVE_CLASS_REGISTER_STATIC_BLOCK(static_block)
            }
            
            core::Value static_block(core::Frame &frame, std::vector<core::FunctionParameter> parameters) {

                auto strm_out = frame.CreateNewClassWith<false, io::PrintStream>("java.io.PrintStream", [&](io::PrintStream *ref) {
                    ref->SetNativeStream(stdout, true);
                });
                this->SetStaticField("out", strm_out);

                auto strm_err = frame.CreateNewClassWith<false, io::PrintStream>("java.io.PrintStream", [&](io::PrintStream *ref) {
                    ref->SetNativeStream(stderr, true);
                });
                this->SetStaticField("err", strm_err);

                JAVM_NATIVE_CLASS_NO_RETURN
            }
    };

}