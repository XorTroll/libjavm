
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Object {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.lang.Object.registerNatives] called...");
                return ExecutionResult::Void();
            }

            static ExecutionResult getClass(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.lang.Object.getClass] called - array type name: '%s'", str::ToUtf8(TypeUtils::FormatVariableType(this_var)).c_str());
                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    auto ref_type = ReflectionUtils::FindTypeByName(this_obj->GetClassType()->GetClassName());
                    JAVM_LOG("[java.lang.Object.getClass] reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
                    return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(ref_type));
                }
                else if(this_var->CanGetAs<VariableType::Array>()) {
                    auto this_array = this_var->GetAs<type::Array>();
                    
                    auto ref_type = ReflectionUtils::FindArrayType(this_array);
                    JAVM_LOG("[java.lang.Object.getClass] array reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
                    return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(ref_type));
                }
                
                return ExceptionUtils::ThrowInternalException(str::Format("Invalid this variable: %s", str::ToUtf8(TypeUtils::FormatVariableType(this_var)).c_str()));
            }

            static ExecutionResult hashCode(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                return GetObjectHashCode(this_var);
            }

            static ExecutionResult notify(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    auto monitor = this_obj->GetMonitor();
                    monitor->Notify();
                    JAVM_LOG("[java.lang.Object.notify] called...");
                    return ExecutionResult::Void();
                }
                else if(this_var->CanGetAs<VariableType::Array>()) {
                    auto this_array = this_var->GetAs<type::Array>();
                    auto array_obj = this_array->GetObjectInstance();
                    auto monitor = array_obj->GetMonitor();
                    monitor->Notify();
                    JAVM_LOG("[java.lang.Object.notify] called on array...");
                    return ExecutionResult::Void();
                }

                return ExceptionUtils::ThrowInternalException(str::Format("Invalid this variable: %s", str::ToUtf8(TypeUtils::FormatVariableType(this_var)).c_str()));
            }

            static ExecutionResult notifyAll(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    auto monitor = this_obj->GetMonitor();
                    monitor->NotifyAll();
                    JAVM_LOG("[java.lang.Object.notifyAll] called...");
                    return ExecutionResult::Void();
                }
                else if(this_var->CanGetAs<VariableType::Array>()) {
                    auto this_array = this_var->GetAs<type::Array>();
                    auto array_obj = this_array->GetObjectInstance();
                    auto monitor = array_obj->GetMonitor();
                    monitor->NotifyAll();
                    JAVM_LOG("[java.lang.Object.notifyAll] called on array...");
                    return ExecutionResult::Void();
                }

                return ExceptionUtils::ThrowInternalException(str::Format("Invalid this variable: %s", str::ToUtf8(TypeUtils::FormatVariableType(this_var)).c_str()));
            }

            static ExecutionResult wait(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                auto timeout_v = param_vars[0];
                const auto timeout = timeout_v->GetValue<type::Long>();

                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    auto monitor = this_obj->GetMonitor();
                    monitor->WaitFor(timeout);
                    JAVM_LOG("[java.lang.Object.wait] called...");
                    return ExecutionResult::Void();
                }
                else if(this_var->CanGetAs<VariableType::Array>()) {
                    auto this_array = this_var->GetAs<type::Array>();
                    auto array_obj = this_array->GetObjectInstance();
                    auto monitor = array_obj->GetMonitor();
                    monitor->WaitFor(timeout);
                    JAVM_LOG("[java.lang.Object.wait] called on array...");
                    return ExecutionResult::Void();
                }

                return ExceptionUtils::ThrowInternalException(str::Format("Invalid this variable: %s", str::ToUtf8(TypeUtils::FormatVariableType(this_var)).c_str()));
            }
    };

}