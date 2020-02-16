
#pragma once
#include <java/lang/Object.class.hpp>

namespace java::lang {

    class String final : public native::Class {

        private:
            std::string value;

        public:
            std::string GetNativeString() {
                return this->value;
            }

            void SetNativeString(const std::string &str) {
                this->value = str;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(String) {

                JAVM_NATIVE_CLASS_NAME("java.lang.String")

                JAVM_NATIVE_CLASS_REGISTER_METHOD(length)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(charAt)

            }

            core::Value length(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<String>(this_v);

                auto len = this_ref->GetNativeString().length();
                return core::CreateNewValue<int>(len);
            }

            core::Value charAt(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<String>(this_v);
                
                auto index = parameters[0].value->Get<int>();
                auto char_at = this_ref->GetNativeString()[index];
                return core::CreateNewValue<char>(char_at);
            }
            
    };

}