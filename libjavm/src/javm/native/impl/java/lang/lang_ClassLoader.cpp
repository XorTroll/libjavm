#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_ClassLoader.hpp> 

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult ClassLoader::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.ClassLoader.registerNatives] called...");
        return ExecutionResult::Void();
    }

}