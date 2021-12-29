
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::reflect {

    using namespace vm;

    class Reflection {
        public:
            static ExecutionResult getCallerClass(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getClassAccessFlags(const std::vector<Ptr<Variable>> &param_vars);
    };

}