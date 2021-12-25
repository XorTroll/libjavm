
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::vm {

    class ExceptionUtils {
        private:
            static Ptr<Variable> CreateThrowableImpl(const String &class_name) {
                auto class_type = inner_impl::LocateClassTypeImpl(class_name);
                if(class_type) {
                    return inner_impl::NewClassVariableImpl(class_type);
                }
                return nullptr;
            }

        public:
            static ExecutionResult ThrowWithType(const String &class_name) {
                auto throwable_v = CreateThrowableImpl(class_name);
                if(throwable_v) {
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
                    auto ret = throwable_obj->CallConstructor(throwable_v, u"()V");
                    if(!ret.IsInvalidOrThrown()) {
                        inner_impl::NotifyExceptionThrownImpl(throwable_v);
                        return ExecutionResult::Thrown();
                    }
                }
                return ExecutionResult::InvalidState();
            }

            static ExecutionResult ThrowWithTypeAndMessage(const String &class_name, const String &msg) {
                auto throwable_v = CreateThrowableImpl(class_name);
                if(throwable_v) {
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
                    auto msg_v = inner_impl::CreateNewString(msg);
                    const auto ret = throwable_obj->CallConstructor(throwable_v, u"(Ljava/lang/String;)V", msg_v);
                    if(!ret.IsInvalidOrThrown()) {
                        inner_impl::NotifyExceptionThrownImpl(throwable_v);
                        return ExecutionResult::Thrown();
                    }
                }
                return ExecutionResult::InvalidState();
            }

            static inline ExecutionResult ThrowInternalException(const String &msg) {
                return ThrowWithTypeAndMessage(u"java/lang/RuntimeException", u"[JAVM-INTERNAL] " + msg);
            }

            static inline Ptr<Variable> CreateThrowable(const String &class_name, const String &msg = u"") {
                auto throwable_v = CreateThrowableImpl(class_name);
                if(throwable_v) {
                    auto throwable_obj = throwable_v->GetAs<type::ClassInstance>();
                    if(msg.empty()) {
                        const auto ret = throwable_obj->CallConstructor(throwable_v, u"()V");
                        if(ret.IsInvalidOrThrown()) {
                            return nullptr;
                        }
                    }
                    else {
                        auto msg_v = inner_impl::CreateNewString(msg);
                        const auto ret = throwable_obj->CallConstructor(throwable_v, u"(Ljava/lang/String;)V", msg_v);
                        if(ret.IsInvalidOrThrown()) {
                            return nullptr;
                        }
                    }
                }
                return throwable_v;
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

    namespace jstr {

        static void Assign(Ptr<Variable> str_var, const String &native_str) {
            if(str_var->CanGetAs<VariableType::ClassInstance>()) {
                auto str_obj = str_var->GetAs<type::ClassInstance>();
                const size_t str_len = native_str.length();

                auto arr_var = TypeUtils::NewArray(str_len, VariableType::Character);
                auto arr_obj = arr_var->GetAs<type::Array>();

                for(u32 i = 0; i < str_len; i++) {
                    auto char_var = TypeUtils::NewPrimitiveVariable<type::Character>(native_str[i]);
                    arr_obj->SetAt(i, char_var);
                }

                str_obj->SetField(u"value", u"[C", arr_var);
            }
        }

        static String GetValue(Ptr<Variable> str_var) {
            if(!str_var) {
                // TODO
                return u"<invalid-str-var>";
            }
            if(str_var->IsNull()) {
                // TODO
                return u"<null>";
            }
            
            String ret_str;
            
            if(str_var->CanGetAs<VariableType::ClassInstance>()) {
                auto str_obj = str_var->GetAs<type::ClassInstance>();

                auto arr_var = str_obj->GetField(u"value", u"[C");
                auto arr_obj = arr_var->GetAs<type::Array>();

                for(u32 i = 0; i < arr_obj->GetLength(); i++) {
                    auto char_var = arr_obj->GetAt(i);
                    const auto char_val = char_var->GetValue<type::Character>();
                    ret_str += static_cast<char16_t>(char_val);
                }
            }

            return ret_str;
        }

        static Ptr<Variable> CheckInternValue(const String &native_str) {
            for(auto &var: inner_impl::GetInternStringTable()) {
                const auto value_val = GetValue(var);
                if(native_str == value_val) {
                    return var;
                }
            }
            return nullptr;
        }
        
        static Ptr<Variable> CheckIntern(Ptr<Variable> str_var) {
            const auto value = GetValue(str_var);
            auto intern_v = CheckInternValue(value);
            if(intern_v) {
                return intern_v;
            }
            // String isn't cached, so cache it now
            inner_impl::AddInternString(str_var);
            return str_var;
        }

        static Ptr<Variable> CreateNew(const String &native_str) {
            // First, look if it's already cached
            auto str_var = CheckInternValue(native_str);
            if(str_var) {
                return str_var;
            }

            auto str_class_type = inner_impl::LocateClassTypeImpl(u"java/lang/String");
            if(str_class_type) {
                if(native_str.empty()) {
                    // Trick to avoid infinite loop of String creation:
                    // Java's String <init> constructor assigns it the value of a "" String's value.
                    // Always calling the constructor would also call it for the "" String, calling the same code indefinitely until a segfault happens.
                    // Thus, we don't call the constructor here
                    str_var = TypeUtils::NewClassVariable(str_class_type);
                }
                else {
                    str_var = TypeUtils::NewClassVariable(str_class_type, u"()V");
                }
                Assign(str_var, native_str);
            }

            // String isn't cached, so cache it now
            inner_impl::AddInternString(str_var);

            return str_var;
        }

        static inline Ptr<Variable> CreateNewUtf8(const std::string &native_str) {
            return CreateNew(str::FromUtf8(native_str));
        }

    }

    namespace inner_impl {

        Ptr<Variable> CreateNewString(const String &native_str) {
            return jstr::CreateNew(native_str);
        }

        String GetStringValue(Ptr<Variable> str) {
            return jstr::GetValue(str);
        }

        ExecutionResult ThrowWithTypeImpl(const String &class_name) {
            return ExceptionUtils::ThrowWithType(class_name);
        }

        ExecutionResult ThrowWithTypeAndMessageImpl(const String &class_name, const String &msg) {
            return ExceptionUtils::ThrowWithTypeAndMessage(class_name, msg);
        }

    }

}