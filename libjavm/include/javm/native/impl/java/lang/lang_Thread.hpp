
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Thread {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult currentThread(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult setPriority0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult isAlive(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult start0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}