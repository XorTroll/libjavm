// Define this to enable debug logs (tons of output, only useful for development!)
// #define JAVM_DEBUG_LOG

#include <javm/javm_VM.hpp>

// We will use JARs
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// Use libnx-specific threads/sync
#include <nx_LibnxThread.hpp>
#include <nx_LibnxSync.hpp>

PadState g_hid_pad;

void DoExit()  {
    // Let's make sure everything we've printed so far gets shown
    consoleUpdate(nullptr);

    // Wait until user presses A, then exit
    while(appletMainLoop()) {
        padUpdate(&g_hid_pad);

        if(padGetButtonsDown(&g_hid_pad) & HidNpadButton_A) {
            break;
        }
    }

    consoleExit(nullptr);
    exit(0);
}

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
        DoExit();
    }
    else if(res.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        DoExit();
    }
}

// In this example, the homebrew will read rt.jar and an entrypoint JAR from the SD card

// Define the initial/base system properties of our VM, which are needed for the VM setup
inline vm::PropertyTable GetInitialSystemProperties() {
    const auto hos_ver = hosversionGet();
    const auto os_version = str::Format("%d.%d.%d", HOSVER_MAJOR(hos_ver), HOSVER_MINOR(hos_ver), HOSVER_MICRO(hos_ver));

    return vm::PropertyTable {
        { u"path.separator", u":" },
        { u"file.encoding.pkg", u"sun.io" },
        { u"os.arch", u"aarch64" },
        { u"os.name", u"horizon-nx" },
        { u"os.version", os_version },
        { u"line.separator", u"\n" },
        { u"file.separator", u"/" },
        { u"sun.jnu.encoding", u"UTF-8" },
        { u"file.encoding", u"UTF-8" },
    };
}

int main(int argc, char **argv) {
    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_hid_pad);

    // Add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/javm-libnx/rt.jar"); // Java standard library JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/javm-libnx/entry.jar"); // Entrypoint JAR

    if(!main_jar->CanBeExecuted()) {
        // The JAR failed to load or it doesn't specify a main class (is an invalid file, or a JAR library)
        printf("The JAR file can't be executed or is not an executable JAR...\n");
        DoExit();
        return 0;
    }

    // Initial VM preparation (must be called ONCE)
    rt::InitializeVM(GetInitialSystemProperties());

    // Prepare execution, which must be done here (before any executions) and/or after having called ResetExecution()
    const auto res = rt::PrepareExecution();
    CheckHandleException(res);

    // Create a Java string array (String[]) and populate it with some dummy values
    constexpr const char *DummyArgs[] = { "a", "b", "c", "d", "1", "2", "3", "4" };
    constexpr size_t DummyArgCount = sizeof(DummyArgs) / sizeof(const char*);
    auto args_arr_v = vm::NewArrayVariable(DummyArgCount, rt::LocateClassType(u"java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(size_t i = 0; i < DummyArgCount; i++) {
        args_arr_obj->SetAt(i, vm::jutil::NewUtf8String(DummyArgs[i]));
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

    DoExit();
    return 0;
}