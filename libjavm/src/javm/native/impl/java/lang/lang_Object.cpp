#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_Object.hpp>
#include <javm/native/impl/impl_Base.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult Object::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Object.registerNatives] called...");
        return ExecutionResult::Void();
    }

    ExecutionResult Object::getClass(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Object.getClass] called - array type name: '%s'", str::ToUtf8(FormatVariableType(this_var)).c_str());
        if(this_var->CanGetAs<VariableType::ClassInstance>()) {
            auto this_obj = this_var->GetAs<type::ClassInstance>();
            auto ref_type = ref::FindReflectionTypeByName(this_obj->GetClassType()->GetClassName());
            JAVM_LOG("[java.lang.Object.getClass] reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
            return ExecutionResult::ReturnVariable(NewClassTypeVariable(ref_type));
        }
        else if(this_var->CanGetAs<VariableType::Array>()) {
            auto this_array = this_var->GetAs<type::Array>();
            
            auto ref_type = ref::FindArrayReflectionType(this_array);
            JAVM_LOG("[java.lang.Object.getClass] array reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
            return ExecutionResult::ReturnVariable(NewClassTypeVariable(ref_type));
        }
        
        return ThrowInternal(str::Format("Invalid this variable: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
    }

    ExecutionResult Object::hashCode(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        return GetObjectHashCode(this_var);
    }

    ExecutionResult Object::notify(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
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

        return ThrowInternal(str::Format("Invalid this variable: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
    }

    ExecutionResult Object::notifyAll(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
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

        return ThrowInternal(str::Format("Invalid this variable: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
    }

    ExecutionResult Object::wait(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
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

        return ThrowInternal(str::Format("Invalid this variable: %s", str::ToUtf8(FormatVariableType(this_var)).c_str()));
    }

}