# libjavm

> Simple, compact, zero-dependency C++17 Java Virtual Machine library

The only "dependencies" it does require are standard C/C++17 libraries, plus the offial Java standard library (`rt.jar` file) since virtually all compiled Java uses it.

Note that, for threading and synchronization items (mutexes, condvars...) you must provide your own implementation. Nevertheless, libjavm provides a default implementation with **pthread** for threading and **standard C++** for sync stuff (`pthread_t`, `std::recursive_mutex`, `std::condition_variable_any`...)

It provides everything necessary to run Java (8 or lower...?) code in any kind of system.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), as the original base for the project.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library.

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for a lot of aspects of the VM.

- [Official Oracle VM specs](https://docs.oracle.com/javase/specs/jvms/se11/jvms11.pdf) (although these are for Java 11)

## Usage

Check the [examples](examples) directory for some example programs using this library.

> TODO: proper documentation...

## Comparison with JRE

The tests used to test the VM (slightly modified KiVM tests) are located at [javm-test-suite](javm-test-suite). Currently 26 out of 28 tests are successfully passed (comparing their output with JRE):

- `ArgumentTest`: pass!

- `ArithmeticTest`: pass!

- `ArrayTest`: pass!

- `ArrayTest1`: pass!

- `ArrayTest2`: pass!

- `AssertTest`: pass!

- `ChineseTest`: pass!

- `ClassCastTest`: pass!

- `ClassNameTest`: pass!

- `CovScriptJNITest`: pass!

- `ExceptionTest`: pass!

- `ExceptionTest1`: pass!

- `ExceptionTest2`: pass!

- `ExceptionTest3`: pass!

- `FileTest`: fail (filesystem-related native methods aren't implemented)

- `GCTest`: pass!

- `HashTest`: pass!

- `HelloWorld`: pass!

- `LambdaTest`: fail (dynamic stuff not implemented yet)

- `Main`: pass!

- `PackagePrivateTest`: pass!

- `Polymorphism`: pass!

- `StaticFieldTest`: pass!

- `StaticResolution`: pass!

- `StringBuilderTest1`: pass!

- `ThreadExceptionTest`: pass!

- `ThreadTest`: pass!

## TO-DO list

- Implement `invokedynamic`, `wide`, `tableswitch` opcodes

- Implement not implemented standard native methods (only implemented basic ones to get past initialization, for now)

- Dynamic invoking stuff, related to `invokedynamic` opcode (method handle support, constant pool items related to this...)

- Support or take into account other annotations (currently only `CallerSensitive` is checked)

- The many `TODO` comments spread in code

- (...)