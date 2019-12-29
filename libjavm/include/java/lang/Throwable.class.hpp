
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class Throwable : public Object {

        private:
            std::string msg;

        public:
            Throwable() : Object("java.lang.Throwable") {
                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(getMessage)
            }

            std::string GetMessage() {
                return this->msg;
            }

            void SetMessage(std::string message) {
                this->msg = message;
            }

            JAVM_NATIVE_CLASS_CTOR(Throwable)

            core::ValuePointerHolder constructor(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<Throwable>(this_param.value);
                switch(parameters.size()) {
                    case 1: {
                        if(parameters[0].value.IsValidCast<String>()) {
                            auto val_str = parameters[0].value.GetReference<String>();
                            auto msg_str = val_str->GetString();
                            this_ref->SetMessage(msg_str);
                        }
                        break;
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::ValuePointerHolder getMessage(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<Throwable>(this_param.value);
                auto msg = this_ref->GetMessage();

                auto str_obj = core::ValuePointerHolder::Create<String>();
                auto str_ref = str_obj.GetReference<String>();
                str_ref->SetString(msg);
                return str_obj;
            }
    };

}