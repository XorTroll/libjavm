
#pragma once
#include <java/lang/String.class.hpp>

namespace java::io {

    class PrintStream final : public native::Class {

        private:
            FILE *stream;
            bool is_std; // Avoid fclosing stdout or stderr...

            bool DoPrint(FILE *this_stream, core::FunctionParameter param) {
                switch(param.parsed_type) {
                    case core::ValueType::Boolean: {
                        bool b = param.value->Get<bool>();
                        fprintf(this_stream, "%s", (b ? "true" : "false"));
                        return true;
                    }
                    case core::ValueType::Character: {
                        char c = param.value->Get<char>();
                        fprintf(this_stream, "%c", c);
                        return true;
                    }
                    case core::ValueType::Float: {
                        float f = param.value->Get<float>();
                        fprintf(this_stream, "%f", f);
                        return true;
                    }
                    case core::ValueType::Double: {
                        double d = param.value->Get<double>();
                        fprintf(this_stream, "%lf", d);
                        return true;
                    }
                    case core::ValueType::Integer: {
                        int i = param.value->Get<int>();
                        fprintf(this_stream, "%d", i);
                        return true;
                    }
                    case core::ValueType::Long: {
                        long l = param.value->Get<long>();
                        fprintf(this_stream, "%ld", l);
                        return true;
                    }
                    case core::ValueType::ClassObject: {
                        if(param.value->IsValidCast<lang::String>()) {
                            auto str_ref = param.value->GetReference<lang::String>();
                            auto str = str_ref->GetNativeString();
                            
                            fprintf(this_stream, "%s", str.c_str());
                            return true;
                        }
                        break;
                    }
                    default:
                        break;
                }
                return false;
            }

        public:
            FILE *GetNativeStream() {
                return this->stream;
            }

            void SetNativeStream(FILE *strm, bool isstd = false) {
                this->stream = strm;
                this->is_std = isstd;
            }

        public:
            JAVM_NATIVE_CLASS_CTOR(PrintStream) : stream(nullptr), is_std(false) {

                JAVM_NATIVE_CLASS_NAME("java.io.PrintStream")

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

            core::Value constructor(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<PrintStream>(this_v);
                if(parameters.size() == 1) {
                    if(parameters[0].value->IsValidCast<lang::String>()) {
                        auto str_ref = parameters[0].value->GetReference<lang::String>();
                        auto str = str_ref->GetNativeString();
                        
                        FILE *f = fopen(str.c_str(), "a+");
                        this_ref->SetNativeStream(f);
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

            core::Value print(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<PrintStream>(this_v);

                auto this_stream = this_ref->GetNativeStream();
                if(this_stream != nullptr) {
                    if(parameters.size() == 1) {
                        this->DoPrint(this_stream, parameters[0]);
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }
            
            core::Value println(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                auto this_ref = this->GetThisInstance<PrintStream>(this_v);

                auto this_stream = this_ref->GetNativeStream();
                if(this_stream != nullptr) {
                    if(parameters.size() == 1) {
                        auto printed = this->DoPrint(this_stream, parameters[0]);
                        if(printed) {
                            // Print the extra line
                            fprintf(this_stream, "\n");
                        }
                    }
                }

                JAVM_NATIVE_CLASS_NO_RETURN
            }

    };

}