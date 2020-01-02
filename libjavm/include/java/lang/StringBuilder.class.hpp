
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class StringBuilder : public Object {

        private:
            std::string value;

        public:
            std::string GetNativeString() {
                return this->value;
            }

            void SetNativeString(std::string str) {
                this->value = str;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(StringBuilder) {

                JAVM_NATIVE_CLASS_NAME("java.lang.StringBuilder")

                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(append)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(toString)

            }

            core::Value constructor(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisReference<StringBuilder>(this_param);
                switch(parameters.size()) {
                    case 1: {
                        auto initial_arg = parameters[0].value;
                        if(initial_arg->IsValidCast<String>()) {
                            // Initial string
                            auto str_ref = initial_arg->GetReference<String>();
                            this_ref->SetNativeString(str_ref->GetNativeString());
                        }
                        else if(initial_arg->IsValidCast<int>()) {
                            // Initial capacity
                            std::string str;
                            str.reserve(initial_arg->Get<int>());
                            this_ref->SetNativeString(str);
                        }
                        break;
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value append(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisReference<StringBuilder>(this_param);
                if(parameters[0].value->IsValidCast<String>()) {
                    auto str_ref = parameters[0].value->GetReference<String>();
                    
                    auto cur_str = this_ref->GetNativeString();
                    this_ref->SetNativeString(cur_str + str_ref->GetNativeString());
                }
                else if(parameters[0].value->IsValidCast<int>()) {
                    int i_val = parameters[0].value->Get<int>();
                    if(parameters[0].parsed_type == core::ValueType::Character) {
                        auto chr = (char)i_val;

                        auto cur_str = this_ref->GetNativeString();
                        this_ref->SetNativeString(cur_str + chr);
                    }
                    else if(parameters[0].parsed_type == core::ValueType::Boolean) {
                        auto boolean = (bool)i_val;

                        auto cur_str = this_ref->GetNativeString();
                        this_ref->SetNativeString(cur_str + (boolean ? "true" : "false"));
                    }
                    else {
                        auto cur_str = this_ref->GetNativeString();
                        this_ref->SetNativeString(cur_str + std::to_string(i_val));
                    }
                }
                else {
                    // TODO: more...?
                }

                return this_param.value;
            }

            core::Value toString(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisReference<StringBuilder>(this_param);
                auto str = this_ref->GetNativeString();

                auto str_obj = frame.CreateNewClassWith<true, String>("java.lang.String", [&](String *ref) {
                    ref->SetNativeString(str);
                });

                return str_obj;
            }

    };

}