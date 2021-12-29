
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Throwable {
        public:
            static ExecutionResult fillInStackTrace(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getStackTraceDepth(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getStackTraceElement(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}