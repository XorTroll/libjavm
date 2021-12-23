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

void CheckHandleException(vm::ExecutionResult ret) {
    if(ret.Is<vm::ExecutionStatus::Thrown>()) {
        auto [thread, throwable_var] = vm::ThreadUtils::GetThrownExceptionInfo();
        auto throwable_obj = throwable_var->GetAs<vm::type::ClassInstance>();
        auto msg_v = throwable_obj->GetField(u"detailMessage", u"Ljava/lang/String;");
        auto msg = u"Exception in thread \"" + thread->GetThreadName() + u"\" " + vm::TypeUtils::FormatVariableType(throwable_var);
        const auto msg_str = vm::StringUtils::GetValue(msg_v);
        if(!msg_str.empty()) {
            msg +=  + u": " + vm::StringUtils::GetValue(msg_v);
        }
        printf("%s\n", StrUtils::ToUtf8(msg).c_str());
        for(auto call_info: thread->GetInvertedCallStack()) {
            printf("    at %s.%s%s\n", StrUtils::ToUtf8(call_info.caller_type->GetClassName()).c_str(), StrUtils::ToUtf8(call_info.invokable_name).c_str(), StrUtils::ToUtf8(call_info.invokable_desc).c_str());
        }
        DoExit();
    }
    else if(ret.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        DoExit();
    }
}

// In this example, the homebrew will read rt.jar and an entrypoint JAR from the SD card

// Define the initial/base system properties of our VM, which are needed for the VM setup
inline vm::PropertyTable GetInitialSystemProperties() {
    const auto hos_ver = hosversionGet();
    const auto os_version = StrUtils::Format("%d.%d.%d", HOSVER_MAJOR(hos_ver), HOSVER_MINOR(hos_ver), HOSVER_MICRO(hos_ver));

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

    // 1) add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/javm-libnx/rt.jar"); // Java standard library JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/javm-libnx/entry.jar"); // Entrypoint JAR

    // 2) initial VM preparation (must be called ONCE)
    rt::InitializeVM(GetInitialSystemProperties());

    // 3) prepare execution, which must be done here (before any executions) and/or after having called ResetExecution()
    const auto ret = rt::PrepareExecution();
    CheckHandleException(ret);

    // Create a Java string array (String[]) and populate it with some dummy values
    constexpr const char *DummyArgs[] = { "a", "b", "c", "d", "1", "2", "3", "4" };
    constexpr size_t DummyArgCount = sizeof(DummyArgs) / sizeof(const char*);
    auto args_arr_v = vm::TypeUtils::NewArray(DummyArgCount, rt::LocateClassType(u"java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(size_t i = 0; i < DummyArgCount; i++) {
        args_arr_obj->SetAt(i, vm::StringUtils::CreateNew(StrUtils::FromUtf8(DummyArgs[i])));
    }

    if(main_jar->CanBeExecuted()) {
        auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
        const auto ret = main_class_type->CallClassMethod(u"main", u"([Ljava/lang/String;)V", args_arr_v);
        CheckHandleException(ret);
    }
    else {
        printf("The JAR file can't be executed or is not an executable JAR...\n");
    }

    DoExit();
    return 0;
}