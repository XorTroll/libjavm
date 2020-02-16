# libjavm

libjavm is a small, header-only, zero-dependency C++17 Java Virtual Machine library. The only "dependency" it requires is standard C and C++17 libraries.

It provides everything necessary to run Java (<=8) code in any kind of system.

The only thing necessary is a proper standard library, whose native interface is being worked on as an optional part of the library.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), since this project is basically a quite extended C++ port of that VM project, and started being a simple port of it.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for certain tasks (mainly the implementation of certain instructions)

## Usage

This is a quick demo of how libjavm works:

```cpp
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

        // Execute main(...) on the JAR's main class
        auto ret = machine.CallFunction(jar_ref->GetMainClass(), "main", args_array_value);
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
```

### For a more deep explanation and/or documentation, go [here](docs/Start.md)!

## TO-DO list

- [ ] Implement all opcodes/instructions (very, very few missing)

- [ ] Implement a basic standard library (barely started, this will take a long time)

## Standard library

### Implemented types

- `java.lang.Object` (some methods not implemented yet)

- `java.lang.String` (some methods not implemented yet)

- `java.lang.Enum` (few methods not implemented yet)

- `java.lang.StringBuilder` (barely implemented constructor, `append` and `toString`)

- `java.io.PrintStream` (implemented String-based constructor, `print` and `println`)

- `java.lang.System` (implemented `out` and `err` static streams to write to console, `arraycopy`...)

- `java.lang.Throwable` (implemented String-based constructor and `getMessage`)

- Some exception/error types: `Exception`, `Error`, `RuntimeException`, `IllegalArgumentException`, `CloneNotSupportedException`, `LinkageError`, `NoClassDefFoundError`