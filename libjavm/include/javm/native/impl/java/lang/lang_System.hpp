
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class System {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult initProperties(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult arraycopy(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult setIn0(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult setOut0(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult setErr0(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult mapLibraryName(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult loadLibrary(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult currentTimeMillis(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult identityHashCode(const std::vector<Ptr<Variable>> &param_vars);
    };

}