
#pragma once
#include <java/lang/String.class.hpp>

namespace java::io {

    class PrintStream : public lang::Object {

        private:
            FILE *stream;
            bool is_std; // Avoid fclosing stdout or stderr...

        public:
            PrintStream() : Object("java.io.PrintStream"), stream(nullptr), is_std(false) {
                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(print)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(println)
            }

            ~PrintStream() {
                if(!this->is_std) {
                    if(this->stream != nullptr) {
                        fclose(this->stream);
                    }
                }
            }

            FILE *GetNativeStream() {
                return this->stream;
            }

            void SetNativeStream(FILE *strm, bool isstd = false) {
                this->stream = strm;
                this->is_std = isstd;
            }

            JAVM_NATIVE_CLASS_CTOR(PrintStream)

            core::ValuePointerHolder constructor(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<PrintStream>();
                if(parameters.size() == 1) {
                    if(parameters[0].value.IsValidCast<lang::String>()) {
                        auto str_ref = parameters[0].value.GetReference<lang::String>();
                        auto str = str_ref->GetString();
                        
                        FILE *f = fopen(str.c_str(), "a+");
                        this_ref->SetNativeStream(f);
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::ValuePointerHolder print(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<PrintStream>();

                auto this_stream = this_ref->GetNativeStream();
                if(this_stream != nullptr) {
                    if(parameters.size() == 1) {
                        switch(parameters[0].parsed_type) {
                            case core::ValueType::Boolean: {
                                bool b = parameters[0].value.Get<bool>();
                                fprintf(this_stream, "%s", (b ? "true" : "false"));
                                break;
                            }
                            case core::ValueType::Character: {
                                char c = parameters[0].value.Get<char>();
                                fprintf(this_stream, "%c", c);
                                break;
                            }
                            case core::ValueType::Float: {
                                float f = parameters[0].value.Get<float>();
                                fprintf(this_stream, "%f", f);
                                break;
                            }
                            case core::ValueType::Double: {
                                double d = parameters[0].value.Get<double>();
                                fprintf(this_stream, "%lf", d);
                                break;
                            }
                            case core::ValueType::Integer: {
                                int i = parameters[0].value.Get<int>();
                                fprintf(this_stream, "%d", i);
                                break;
                            }
                            case core::ValueType::Long: {
                                long l = parameters[0].value.Get<long>();
                                fprintf(this_stream, "%ld", l);
                                break;
                            }
                            case core::ValueType::ClassObject: {
                                if(parameters[0].value.IsValidCast<lang::String>()) {
                                    auto str_ref = parameters[0].value.GetReference<lang::String>();
                                    auto str = str_ref->GetString();
                                    
                                    fprintf(this_stream, "%s", str.c_str());
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }
            
            core::ValuePointerHolder println(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this_param.value.GetReference<PrintStream>();

                auto this_stream = this_ref->GetNativeStream();
                if(this_stream != nullptr) {
                    if(parameters.size() == 1) {
                        switch(parameters[0].parsed_type) {
                            case core::ValueType::Boolean: {
                                bool b = parameters[0].value.Get<bool>();
                                fprintf(this_stream, "%s\n", (b ? "true" : "false"));
                                break;
                            }
                            case core::ValueType::Character: {
                                char c = parameters[0].value.Get<char>();
                                fprintf(this_stream, "%c\n", c);
                                break;
                            }
                            case core::ValueType::Float: {
                                float f = parameters[0].value.Get<float>();
                                fprintf(this_stream, "%f\n", f);
                                break;
                            }
                            case core::ValueType::Double: {
                                double d = parameters[0].value.Get<double>();
                                fprintf(this_stream, "%lf\n", d);
                                break;
                            }
                            case core::ValueType::Integer: {
                                int i = parameters[0].value.Get<int>();
                                fprintf(this_stream, "%d\n", i);
                                break;
                            }
                            case core::ValueType::Long: {
                                long l = parameters[0].value.Get<long>();
                                fprintf(this_stream, "%ld\n", l);
                                break;
                            }
                            case core::ValueType::ClassObject: {
                                if(parameters[0].value.IsValidCast<lang::String>()) {
                                    auto str_ref = parameters[0].value.GetReference<lang::String>();
                                    auto str = str_ref->GetString();
                                    fprintf(this_stream, "%s\n", str.c_str());
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }
    };

}