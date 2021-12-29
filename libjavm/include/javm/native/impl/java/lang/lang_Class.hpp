
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Class {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getPrimitiveClass(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult desiredAssertionStatus0(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult forName0(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getDeclaredFields0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult isInterface(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult isPrimitive(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult isAssignableFrom(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getModifiers(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}