
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::vm {

    class ExceptionUtils {

        private:
            static Ptr<Variable> CreateThrowableByType(const std::string &class_name) {
                auto class_type = inner_impl::LocateClassTypeImpl(class_name);
                if(class_type) {
                    return inner_impl::NewClassVariableImpl(class_type);
                }
                return nullptr;
            }

        public:
            static ExecutionResult ThrowWithType(const std::string &class_name) {
                auto throwable_v = CreateThrowableByType(class_name);
                if(throwable_v) {
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
                    auto ret = throwable_obj->CallConstructor(throwable_v, "()V");
                    if(ret.IsInvalidOrThrown()) {
                        return ret;
                    }
                    return ExecutionResult::ReturnVariable(throwable_v);
                }
                // Don't throw another exception, since if basic Java exception types aren't present we might end up in a recursive loop
                return ExecutionResult::InvalidState();
            }

            static ExecutionResult ThrowWithTypeAndMessage(const std::string &class_name, const std::string &msg) {
                auto throwable_v = CreateThrowableByType(class_name);
                if(throwable_v) {
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
                    auto msg_v = inner_impl::CreateNewString(msg);
                    auto ret = throwable_obj->CallConstructor(throwable_v, "(Ljava/lang/String;)V", msg_v);
                    if(ret.IsInvalidOrThrown()) {
                        return ret;
                    }
                    return ExecutionResult::ReturnVariable(throwable_v);
                }
                // Don't throw another exception, since if basic Java exception types aren't present we might end up in a recursive loop
                return ExecutionResult::InvalidState();
            }

    };

    namespace inner_impl {

        static inline std::vector<Ptr<Variable>> g_intern_string_table;

        static inline std::vector<Ptr<Variable>> GetInternStringTable() {
            return g_intern_string_table;
        }

        static inline void AddInternString(Ptr<Variable> str_var) {
            g_intern_string_table.push_back(str_var);
        }

    }

    class StringUtils {

        public:
            static void Assign(Ptr<Variable> str_var, const std::string &native_str) {
                if(str_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto str_obj = str_var->GetAs<type::ClassInstance>();
                    const size_t str_len = native_str.length();

                    auto arr_var = TypeUtils::NewArray(str_len, VariableType::Character);
                    auto arr_obj = arr_var->GetAs<type::Array>();

                    for(u32 i = 0; i < str_len; i++) {
                        auto char_var = TypeUtils::NewPrimitiveVariable<type::Character>(native_str[i]);
                        arr_obj->SetAt(i, char_var);
                    }

                    str_obj->SetField("value", "[C", arr_var);
                }
            }

            static std::string GetValue(Ptr<Variable> str_var) {
                if(!str_var) {
                    return "<invalid-str-var>";
                }
                if(str_var->IsNull()) {
                    return "<null>";
                }
                
                std::string ret_str;
                
                if(str_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto str_obj = str_var->GetAs<type::ClassInstance>();

                    auto arr_var = str_obj->GetField("value", "[C");
                    auto arr_obj = arr_var->GetAs<type::Array>();

                    for(u32 i = 0; i < arr_obj->GetLength(); i++) {
                        auto char_var = arr_obj->GetAt(i);
                        auto char_val = char_var->GetValue<type::Character>();
                        ret_str += char_val;
                    }
                }

                return ret_str;
            }

            static Ptr<Variable> CheckInternValue(const std::string &native_str) {
                for(auto &var: inner_impl::GetInternStringTable()) {
                    auto value_val = GetValue(var);
                    if(native_str == value_val) {
                        return var;
                    }
                }
                return nullptr;
            }
            
            static Ptr<Variable> CheckIntern(Ptr<Variable> str_var) {
                auto value = GetValue(str_var);
                auto intern_v = CheckInternValue(value);
                if(intern_v) {
                    return intern_v;
                }
                // String isn't cached, so cache it now
                inner_impl::AddInternString(str_var);
                return str_var;
            }

            static Ptr<Variable> CreateNew(const std::string &native_str) {
                // First, look if it's already cached
                auto str_var = CheckInternValue(native_str);
                if(str_var) {
                    return str_var;
                }

                auto str_class_type = inner_impl::LocateClassTypeImpl("java/lang/String");
                if(str_class_type) {
                    if(native_str.empty()) {
                        // Trick to avoid infinite loop of String creation:
                        // Java's String <init> constructor assigns it the value of a "" String's value.
                        // Always calling the constructor would also call it for the "" String, calling the same code indefinitely until a segfault happens.
                        // Thus, we don't call the constructor here
                        str_var = TypeUtils::NewClassVariable(str_class_type);
                    }
                    else {
                        str_var = TypeUtils::NewClassVariable(str_class_type, "()V");
                    }
                    Assign(str_var, native_str);
                }
                // String isn't cached, so cache it now
                inner_impl::AddInternString(str_var);

                return str_var;
            }

    };

    namespace inner_impl {

        Ptr<Variable> CreateNewString(const std::string &native_str) {
            return StringUtils::CreateNew(native_str);
        }

        std::string GetStringValue(Ptr<Variable> str) {
            return StringUtils::GetValue(str);
        }

        ExecutionResult ThrowWithTypeImpl(const std::string &class_name) {
            return ExceptionUtils::ThrowWithType(class_name);
        }

        ExecutionResult ThrowWithTypeAndMessageImpl(const std::string &class_name, const std::string &msg) {
            return ExceptionUtils::ThrowWithTypeAndMessage(class_name, msg);
        }

    }

}