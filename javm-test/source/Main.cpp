
#include <javm/core/core_Machine.hpp>
using namespace javm;

int main(int argc, char **argv) {
    
    if(argc < 2) {
        printf("No JAR file specified...\n\n");
        return 1;
    }
    
    std::string jar = argv[1];
    core::Machine machine;
    machine.LoadBuiltinNativeClasses();
    auto &jar_ref = machine.LoadJavaArchive(jar);

    if(jar_ref->CanBeExecuted()) {
        auto args_array_value = core::CreateArray<java::lang::String>(argc - 2);
        auto args_array_ref = args_array_value->GetReference<core::Array>();
        for(int i = 2; i < argc; i++) {
            auto str_arg = core::CreateNewClassWith<true>(machine, "java.lang.String", [&](auto *ref) {
                reinterpret_cast<java::lang::String*>(ref)->SetNativeString(argv[i]);
            });
            args_array_ref->SetAt(i - 2, str_arg);
        }

        auto ret = machine.CallFunction(jar_ref->GetMainClass(), "test1", args_array_value);
        printf("\n ------------------------------------\n\n");

        if(machine.WasExceptionThrown()) {
            auto info = machine.GetExceptionInfo();
            printf("Exception in main thread %s: %s\nExiting VM...\n", info.class_type.c_str(), info.message.c_str());
        }
        else {
            printf("Test suceeded (no exceptions):\n");
            if(ret->IsValid()) {
                if(ret->IsVoid()) {
                    printf("Nothing was returned (void function)\n");
                }
                else {
                    printf("Returned value type: %s\n", core::ClassObject::GetValueName(ret).c_str());
                }
            }
            else {
                printf("An invalid value was returned - an error might have ocurred...\n");
            }
        }
    }
    else {
        printf("The loaded Java file cannot be executed :(\n");
    }

    return 0;
}