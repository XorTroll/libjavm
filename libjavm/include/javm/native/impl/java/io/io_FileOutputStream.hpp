
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileOutputStream {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult writeBytes(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}