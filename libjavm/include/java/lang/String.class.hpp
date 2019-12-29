
#pragma once
#include <java/lang/Object.class.hpp>

namespace java::lang {

    class String : public Object {

        private:
            std::string value;

        public:
            std::string GetString() {
                return this->value;
            }

            void SetString(std::string str) {
                this->value = str;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(String) {

                JAVM_NATIVE_CLASS_NAME("java.lang.String")

                JAVM_NATIVE_CLASS_REGISTER_METHOD(length)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(charAt)

            }

            core::ValuePointerHolder length(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<String>(this_param);

                auto len = this_ref->GetString().length();
                return core::ValuePointerHolder::Create<int>(len);
            }

            core::ValuePointerHolder charAt(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = native::Class::GetThisReference<String>(this_param);
                
                auto index = parameters[0].value.Get<int>();
                auto char_at = this_ref->GetString()[index];
                return core::ValuePointerHolder::Create<char>(char_at);
            }
            
    };

}