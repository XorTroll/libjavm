
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class String {
        public:
            static ExecutionResult intern(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}