#include <javm/javm_VM.hpp>

// We will use/load JAR archives
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// Threading and sync implementations must be included:

// Use default threading implementation (pthread)
#include <javm/extras/extras_PthreadThread.hpp>

// Use default sync implementation (std C++)
#include <javm/extras/extras_CppSync.hpp>

void CheckHandleException(const vm::ExecutionResult res) {
    if(res.Is<vm::ExecutionStatus::Thrown>()) {
        // After retrieving the thrown throwable (the thread is less relevant), the "thrown" state in the VM gets reset so executions are available again
        auto throwable_v = vm::RetrieveThrownThrowable();
        auto thread = vm::RetrieveThrownThread();

        // Therefore, we can call <throwable>.printStackTrace() to print detailed info about the exception
        printf("Got exception in thread \"%s\" (%s)\n", str::ToUtf8(thread->GetThreadName()).c_str(), str::ToUtf8(vm::FormatVariableType(throwable_v)).c_str());
        printf("Printing stack trace:\n");
        printf("---------------------------------------------------------------------------------\n");
        auto throwable_obj = throwable_v->GetAs<vm::type::ClassInstance>();
        throwable_obj->CallInstanceMethod(u"printStackTrace", u"()V", throwable_v);
        printf("---------------------------------------------------------------------------------\n");
        exit(0);
    }
    else if(res.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        exit(0);
    }
}

// In this example, the program is called with Java's standard lib JAR (rt.jar) and another executable JAR to run it, plus optional arguments to be forwarded to Java code

// Define the initial/base system properties of our VM, which are needed for the VM setup
const vm::PropertyTable DemoInitialSystemProperties = {
    { u"path.separator", u":" },
    { u"file.encoding.pkg", u"sun.io" },
    { u"os.arch", u"demo-arch" },
    { u"os.name", u"Demo OS" },
    { u"os.version", u"0.1-demo" },
    { u"line.separator", u"\n" },
    { u"file.separator", u"/" },
    { u"sun.jnu.encoding", u"UTF-8" },
    { u"file.encoding", u"UTF-8" },
};

int main(int argc, char **argv) {
    constexpr auto ExpectedArgCount = 3; // (including the executable itself)
    if(argc < ExpectedArgCount) {
        printf("Expected usage: sample <rt-jar-path> <main-jar-path> [<java-main-args>]\n");
        return 0;
    }

    // Add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[1]); // Java standard library JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[2]); // Entrypoint JAR

    if(!main_jar->CanBeExecuted()) {
        // The JAR failed to load or it doesn't specify a main class (is an invalid file, or a JAR library)
        printf("The JAR file can't be executed or is not an executable JAR...\n");
        return 0;
    }

    // Initial VM preparation (should be called once, although calling it again shouldn't cause any issues)
    rt::InitializeVM(DemoInitialSystemProperties);

    // Prepare execution, which must be done here (before any executions) and/or after having called ResetExecution()
    // This essentially initializes internal standard library components
    const auto res = rt::PrepareExecution();
    CheckHandleException(res);

    const auto args_len = argc - ExpectedArgCount;

    // Create a Java string array (String[]) and populate it
    auto args_arr_v = vm::NewArrayVariable(args_len, rt::LocateClassType(u"java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(auto i = 0; i < args_len; i++) {
        args_arr_obj->SetAt(i, vm::jutil::NewUtf8String(argv[ExpectedArgCount + i]));
    }

    // Find the JAR's main class (specified at MANIFEST.MF)
    auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
    if(main_class_type) {
        // Call the main class's "static void main(String[])" method
        const auto res = main_class_type->CallClassMethod(u"main", u"([Ljava/lang/String;)V", args_arr_v);
        CheckHandleException(res);
    }
    else {
        // Unexpected error finding the class...
        printf("Unexpected error...\n");
    }

    return 0;
}