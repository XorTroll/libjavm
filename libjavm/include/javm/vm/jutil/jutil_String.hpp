
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::vm::jutil {

    Ptr<Variable> NewString(const String &native_str);
    void SetStringValue(Ptr<Variable> str_var, const String &native_str);
    String GetStringValue(Ptr<Variable> str_var);

    void InternString(const String &native_str);
    void InternVariable(Ptr<Variable> str_var);

    inline Ptr<Variable> NewUtf8String(const std::string &native_str) {
        return NewString(str::FromUtf8(native_str));
    }

}