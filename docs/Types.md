# Types

## Java values

In libjavm, Java values consists on `javm::core::Value` instances, which consist on a `std::shared_ptr` of the actual type, `javm::core::ValuePointerHolder`, to have a proper disposing system.

This type takes care of storing a pointer of a certain variable, which can (should) only be usedb with specific types.

### Allowed types

Non-allowed types will be treated as invalid values.

List of allowed types:

- Primitive types, almost equivalent to Java: `int`, `long`, `float`, `double`, `bool` and `char`

- Arrays (`javm::core::Array` class, see below)

- Classes: any kind of class object inheriting from `javm::core::ClassObject` (which can be `javm::core::ClassFile` for loaded bytecode classes (*.class files) or classes inheriting from `javm::native::Class` for native implementations of certain types, like `java::lang::Object` or `java::lang::String`)

Furthermore, there are special values:

- To represent void elements, one can use `javm::core::CreateVoidValue`, which creates a Value of `VoidValue`, an empty type for void values. These are used, for instance, when a native function doesn't return anything (thus, a void function).

- To represent `null` values, one can use `javm::core::CreateNullValue` or the default `Value`/`ValuePointerHolder` constructor, which creates a `null` value.

- The kind of value returned on errors or unexpected cases is an "invalid" value (`javm::core::CreateInvalidValue`), which isn't valid in the execution. Invalid values (for instance, returned when a class isn't found, or an object cannot be created for some reason) are treated like invalid execution states, and an exception would likely be thrown.

### Arrays

The `javm::core::Array` class is a fairly simple class, containing a vector of values, its length and the type of the array.

Arrays can be easily created with `javm::core::CreateArray` functions, with an initial length, or with a list of elements.

Like in Java itself, arrays are not growable. A new one should be created to use a different size.

### Classes

For more detailed information regarding native classes, check the [native API](Native.md).