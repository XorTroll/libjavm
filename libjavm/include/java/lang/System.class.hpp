
#pragma once
#include <java/io/PrintStream.class.hpp>

namespace java::lang {

    class System : public Object {

        public:
            JAVM_NATIVE_CLASS_CTOR(System) {

                JAVM_NATIVE_CLASS_NAME("java.lang.System")

                auto strm_out = core::ValuePointerHolder::Create<io::PrintStream>();
                auto out_ref = strm_out.GetReference<io::PrintStream>();
                out_ref->SetNativeStream(stdout, true);
                this->SetStaticField("out", strm_out);

                auto strm_err = core::ValuePointerHolder::Create<io::PrintStream>();
                auto err_ref = strm_err.GetReference<io::PrintStream>();
                err_ref->SetNativeStream(stderr, true);
                this->SetStaticField("err", strm_err);
            }
            
    };

}