
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <javm/vm/vm_Sync.hpp>
#include <map>

namespace javm::native {

    using NativeInstanceMethod = vm::ExecutionResult(*)(Ptr<vm::Variable>, const std::vector<Ptr<vm::Variable>>&);
    using NativeClassMethod = vm::ExecutionResult(*)(const std::vector<Ptr<vm::Variable>>&);

    void RegisterNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor, NativeInstanceMethod method);
    bool HasNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor);
    NativeInstanceMethod FindNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor);

    void RegisterNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor, NativeClassMethod fn);
    bool HasNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor);
    NativeClassMethod FindNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor);

}