
#pragma once
#include <java/lang/String.class.hpp>

namespace java::lang {

    class StringBuilder : public Object {

        private:
            std::string value;

        public:
            StringBuilder() : Object("java.lang.StringBuilder") {
                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(append)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(toString)
            }
            
            JAVM_NATIVE_CLASS_CTOR(StringBuilder)

            std::string GetString() {
                return this->value;
            }

            void SetString(std::string str) {
                this->value = str;
            }

            core::ValuePointerHolder constructor(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<String>();
                switch(parameters.size()) {
                    case 0: {
                        // Empty constructor, only with 'this'
                        break;
                    }
                    case 1: {
                        auto initial_arg = parameters[0].value;
                        if(initial_arg.IsValidCast<String>()) {
                            // Initial string
                            auto str_ref = initial_arg.GetReference<String>();
                            this_ref->SetString(str_ref->GetString());
                        }
                        else if(initial_arg.IsValidCast<int>()) {
                            // Initial capacity
                            std::string str;
                            str.reserve(initial_arg.Get<int>());
                            this_ref->SetString(str);
                        }
                        break;
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::ValuePointerHolder append(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<String>();
                if(parameters[0].value.IsValidCast<String>()) {
                    auto str_ref = parameters[0].value.GetReference<String>();
                    
                    auto cur_str = this_ref->GetString();
                    this_ref->SetString(cur_str + str_ref->GetString());
                }
                else if(parameters[0].value.IsValidCast<int>()) {
                    if(parameters[0].parsed_type == core::ValueType::Character) {
                        auto chr = (char)parameters[0].value.Get<int>();

                        auto cur_str = this_ref->GetString();
                        this_ref->SetString(cur_str + chr);
                    }
                    else {
                        auto intg = parameters[0].value.Get<int>();

                        auto cur_str = this_ref->GetString();
                        this_ref->SetString(cur_str + std::to_string(intg));
                    }
                }
                else {
                    // TODO: more...?
                }

                return this_param.value;
            }

            core::ValuePointerHolder toString(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<String>();
                auto str = this_ref->GetString();

                auto str_obj = core::ValuePointerHolder::Create<String>();
                auto str_ref = str_obj.GetReference<String>();
                str_ref->SetString(str);
                return str_obj;
            }

    };

}