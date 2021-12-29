
#pragma once
#include <javm/vm/vm_Variable.hpp>
#include <javm/vm/ref/ref_Reflection.hpp>

namespace javm::native::impl {

    using namespace vm;

    ExecutionResult GetObjectHashCode(Ptr<Variable> var);

    Ptr<ref::ReflectionType> GetReflectionTypeFromClassVariable(Ptr<Variable> var);

    ExecutionResult GetClassModifiers(Ptr<Variable> class_var);

}