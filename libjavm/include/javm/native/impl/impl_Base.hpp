
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl {

    using namespace vm;

    ExecutionResult GetObjectHashCode(Ptr<Variable> var) {
        // TODO: consider better hash code (not just the object pointer as an i32...)
        if(var->CanGetAs<VariableType::ClassInstance>()) {
            auto obj = var->GetAs<type::ClassInstance>();
            const auto obj_ptr = reinterpret_cast<uintptr_t>(obj.get());
            const auto hash_code = static_cast<type::Integer>(obj_ptr);
            JAVM_LOG("[java.lang.Object.hashCode] called - hash code: %d", hash_code);
            return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(hash_code));
        }
        else if(var->CanGetAs<VariableType::Array>()) {
            auto array = var->GetAs<type::Array>();
            auto array_obj = array->GetObjectInstance();
            const auto array_obj_ptr = reinterpret_cast<uintptr_t>(array_obj.get());
            const auto hash_code = static_cast<type::Integer>(array_obj_ptr);
            JAVM_LOG("[java.lang.Object.hashCode] called - array hash code: %d", hash_code);
            return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(hash_code));
        }
        
        return ExceptionUtils::ThrowInternalException(str::Format("Invalid hashcode variable: %s", str::ToUtf8(TypeUtils::FormatVariableType(var)).c_str()));
    }

    Ptr<ReflectionType> GetReflectionTypeFromClassVariable(Ptr<Variable> var) {
        if(var->CanGetAs<VariableType::ClassInstance>()) {
            auto var_obj = var->GetAs<type::ClassInstance>();
            auto name_v = var_obj->GetField(u"name", u"Ljava/lang/String;");
            const auto name = jstr::GetValue(name_v);
            return ReflectionUtils::FindTypeByName(name);
        }
        return nullptr;
    }

    inline u16 GetClassModifiersImpl(Ptr<ClassType> class_type) {
        if(class_type) {
            return class_type->GetAccessFlags();
        }
        return static_cast<u16>(AccessFlagUtils::Make(AccessFlags::Public, AccessFlags::Final, AccessFlags::Abstract));
    }

    ExecutionResult GetClassModifiers(Ptr<Variable> class_var) {
        auto ref_type = GetReflectionTypeFromClassVariable(class_var);
        if(ref_type) {
            JAVM_LOG("[native-GetClassModifiers] reflection type name: '%s'", str::ToUtf8(ref_type->GetTypeName()).c_str());
            if(ref_type->IsClassInstance()) {
                auto class_type = ref_type->GetClassType();
                auto flags_v = TypeUtils::NewPrimitiveVariable<type::Integer>(GetClassModifiersImpl(class_type));
                return ExecutionResult::ReturnVariable(flags_v);
            }
        }

        const auto default_flags = static_cast<u16>(AccessFlagUtils::Make(AccessFlags::Abstract, AccessFlags::Final, AccessFlags::Public));
        auto flags_v = TypeUtils::NewPrimitiveVariable<type::Integer>(default_flags);
        return ExecutionResult::ReturnVariable(flags_v);
    }

}