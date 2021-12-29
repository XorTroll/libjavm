
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Object {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getClass(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult hashCode(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult notify(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult notifyAll(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult wait(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}