
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::security {

    using namespace vm;

    class AccessController {
        public:
            static ExecutionResult doPrivileged(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getStackAccessControlContext(const std::vector<Ptr<Variable>> &param_vars);
    };

}