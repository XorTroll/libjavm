
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class Throwable : public Object {

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

            core::Value constructor(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<Throwable>(this_param);
                printf("This class: %s\n", this_ref->GetName().c_str());
                switch(parameters.size()) {
                    case 1: {
                        if(parameters[0].value->IsValidCast<String>()) {
                            auto val_str = parameters[0].value->GetReference<String>();
                            auto msg_str = val_str->GetString();
                            this_ref->SetMessage(msg_str);
                        }
                        break;
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value getMessage(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<Throwable>(this_param);
                auto msg = this_ref->GetMessage();

                auto str_obj = core::CreateNewValue<String>();
                auto str_ref = str_obj->GetReference<String>();
                str_ref->SetString(msg);
                return str_obj;
            }

    };

}