
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class Throwable final : public native::Class {

        private:
            std::string msg;

        public:
            std::string GetMessage() {
                return this->msg;
            }

            void SetMessage(std::string message) {
                this->msg = message;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(Throwable) {

                JAVM_NATIVE_CLASS_NAME("java.lang.Throwable")

                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(getMessage)
            }

            core::Value constructor(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Throwable>(this_v);
                switch(parameters.size()) {
                    case 1: {
                        if(parameters[0].value->IsValidCast<String>()) {
                            auto val_str = parameters[0].value->GetReference<String>();
                            auto msg_str = val_str->GetNativeString();
                            this_ref->SetMessage(msg_str);
                        }
                        break;
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value getMessage(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<Throwable>(this_v);
                auto msg = this_ref->GetMessage();

                auto str_obj = frame.CreateNewClassWith<true, String>("java.lang.String", [&](String *ref) {
                    ref->SetNativeString(msg);
                });

                return str_obj;
            }

    };

}