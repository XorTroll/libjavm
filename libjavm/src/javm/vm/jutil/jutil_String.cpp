#include <javm/javm_VM.hpp>

namespace javm::vm::jutil {

    namespace {

        std::vector<Ptr<Variable>> g_InternStringList;

        Ptr<Variable> TryFindInternString(const String &native_str) {
            for(auto &var: g_InternStringList) {
                const auto value_val = GetStringValue(var);
                if(native_str == value_val) {
                    return var;
                }
            }

            return nullptr;
        }
        
        inline Ptr<Variable> TryFindInternVariable(const Ptr<Variable> &str_var) {
            return TryFindInternString(GetStringValue(str_var));
        }

        Ptr<Variable> NewStringVariable(const String &native_str) {
            auto str_class_type = rt::LocateClassType(u"java/lang/String");
            if(str_class_type) {
                CallerSensitiveGuard guard;

                Ptr<Variable> str_var;
                if(native_str.empty()) {
                    // Trick to avoid infinite loop of String creation:
                    // Java's String <init> constructor assigns it the value of a "" String's value.
                    // Always calling the constructor would also call it for the "" String, calling the same code indefinitely until a segfault happens.
                    // Thus, we don't call the constructor here
                    str_var = NewClassVariable(str_class_type);
                }
                else {
                    str_var = NewClassVariable(str_class_type, u"()V");
                }
                SetStringValue(str_var, native_str);
                return str_var;
            }

            return nullptr;
        }

    }

    Ptr<Variable> NewString(const String &native_str) {
        auto interned_str_var = TryFindInternString(native_str);
        if(interned_str_var) {
            return interned_str_var;
        }

        auto str_var = NewStringVariable(native_str);
        InternVariable(str_var);
        return str_var;
    }

    void SetStringValue(Ptr<Variable> str_var, const String &native_str) {
        if(str_var->CanGetAs<VariableType::ClassInstance>()) {
            auto str_obj = str_var->GetAs<type::ClassInstance>();
            const size_t str_len = native_str.length();

            auto arr_var = NewArrayVariable(str_len, VariableType::Character);
            auto arr_obj = arr_var->GetAs<type::Array>();

            for(u32 i = 0; i < str_len; i++) {
                auto char_var = NewPrimitiveVariable<type::Character>(native_str[i]);
                arr_obj->SetAt(i, char_var);
            }

            str_obj->SetField(u"value", u"[C", arr_var);
        }
    }

    String GetStringValue(Ptr<Variable> str_var) {
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

    void InternString(const String &native_str) {
        // Check if it's already interned
        auto interned_str_var = TryFindInternString(native_str);
        if(!interned_str_var) {
            // String isn't interned, so intern it
            g_InternStringList.push_back(NewStringVariable(native_str));
        }
    }

    void InternVariable(Ptr<Variable> str_var) {
        // Check if it's already interned
        auto interned_str_var = TryFindInternVariable(str_var);
        if(!interned_str_var) {
            // String isn't interned, so intern it
            g_InternStringList.push_back(str_var);
        }
    }

}