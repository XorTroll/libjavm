
#pragma once
#include <java/io/PrintStream.class.hpp>

namespace java::lang {

    class System : public Object {

        public:
            JAVM_NATIVE_CLASS_CTOR(System) {

                JAVM_NATIVE_CLASS_NAME("java.lang.System")

                JAVM_NATIVE_CLASS_REGISTER_STATIC_BLOCK(static_block)
            }
            
            core::Value static_block(core::Frame &frame, std::vector<core::FunctionParameter> parameters) {

                auto strm_out = core::CreateNewValue<io::PrintStream>();
                auto out_ref = strm_out->GetReference<io::PrintStream>();
                out_ref->SetNativeStream(stdout, true);
                this->SetStaticField("out", strm_out);

                auto strm_err = core::CreateNewValue<io::PrintStream>();
                auto err_ref = strm_err->GetReference<io::PrintStream>();
                err_ref->SetNativeStream(stderr, true);
                this->SetStaticField("err", strm_err);

                JAVM_NATIVE_CLASS_NO_RETURN
            }
    };

}