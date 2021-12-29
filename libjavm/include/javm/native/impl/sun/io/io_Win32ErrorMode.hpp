
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::io {

    using namespace vm;

    class Win32ErrorMode {
        public:
            static ExecutionResult setErrorMode(const std::vector<Ptr<Variable>> &param_vars);
    };

}