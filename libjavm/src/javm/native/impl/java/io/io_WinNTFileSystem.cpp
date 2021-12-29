#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/io/io_WinNTFileSystem.hpp>

namespace javm::native::impl::java::io {

    using namespace vm;

    ExecutionResult WinNTFileSystem::initIDs(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.io.WinNTFileSystem.initIDs] called");
        return ExecutionResult::Void();
    }

}