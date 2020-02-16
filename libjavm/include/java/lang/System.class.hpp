
#pragma once
#include <java/io/PrintStream.class.hpp>

namespace java::lang {

    class System final : public native::Class {

        public:
            JAVM_NATIVE_CLASS_CTOR(System) {

                JAVM_NATIVE_CLASS_NAME("java.lang.System")

                JAVM_NATIVE_CLASS_REGISTER_STATIC_BLOCK(static_block)
                JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(arraycopy)
            }
            
            core::Value static_block(core::Frame &frame, std::vector<core::FunctionParameter> parameters) {

                auto strm_out = core::CreateNewClassWith<false>(frame, "java.io.PrintStream", [&](auto *ref) {
                    reinterpret_cast<io::PrintStream*>(ref)->SetNativeStream(stdout, true);
                });
                this->SetStaticField("out", strm_out);

                auto strm_err = core::CreateNewClassWith<false>(frame, "java.io.PrintStream", [&](auto *ref) {
                    reinterpret_cast<io::PrintStream*>(ref)->SetNativeStream(stderr, true);
                });
                this->SetStaticField("err", strm_err);

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value arraycopy(core::Frame &frame, std::vector<core::FunctionParameter> parameters) {

                if(parameters.size() == 5) {
                    auto src_v = parameters[0].value;
                    auto srcpos_v = parameters[1].value;
                    auto dst_v = parameters[2].value;
                    auto dstpos_v = parameters[3].value;
                    auto len_v = parameters[4].value;
                    if(src_v->IsArray()) {
                        auto src = src_v->GetReference<core::Array>();
                        if(srcpos_v->IsValidCast<int>()) {
                            auto srcpos = srcpos_v->Get<int>();
                            if(dst_v->IsArray()) {
                                auto dst = dst_v->GetReference<core::Array>();
                                if(dstpos_v->IsValidCast<int>()) {
                                    auto dstpos = dstpos_v->Get<int>();
                                    if(len_v->IsValidCast<int>()) {
                                        auto len = len_v->Get<int>();
                                        // Create a temporary array, push values there, then move them to the dst array
                                        std::vector<core::Value> tmp_values;
                                        tmp_values.reserve(len);
                                        for(int i = srcpos; i < (srcpos + len); i++) {
                                            tmp_values.push_back(src->GetAt(i));
                                        }
                                        for(int i = 0; i < len; i++) {
                                            dst->SetAt(i + dstpos, tmp_values[i]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }
    };

}