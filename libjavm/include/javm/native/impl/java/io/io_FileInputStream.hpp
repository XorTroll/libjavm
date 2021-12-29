
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileInputStream {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars);
    };

}