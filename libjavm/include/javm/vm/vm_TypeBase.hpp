
#pragma once
#include <javm/vm/vm_ConstantPool.hpp>
#include <map>

namespace javm::vm {

    class ClassType;
    class ClassInstance;
    class Variable;
    class Array;

    // Null object... nothing to store :P
    struct NullObject {};

    enum class VariableType {

        Invalid,
        Byte,
        Boolean,
        Short,
        Character,
        Integer,
        Long,
        Float,
        Double,
        ClassInstance,
        Array,
        NullObject

    };

    namespace type {

        // C++ <-> Java types, only ones usable to create a Variable object.

        // All these 5 types are basically treated as 32-bit signed integers :P

        using Byte = int;
        using Boolean = int;
        using Short = int;
        using Character = int;
        using Integer = int;

        using Long = long;
        using Float = float;
        using Double = double;

        using ClassInstance = ClassInstance;
        using Array = Array;
        using NullObject = NullObject;

    }

    enum class ExecutionStatus {

        Invalid,
        ContinueExecution, // Continue reading instructions
        VoidReturn,
        VariableReturn,
        ThrowableThrown,

    };

    struct ExecutionResult {
        ExecutionStatus status;
        Ptr<Variable> ret_var; // nullptr if void return, variable if var returned, throwable var if thrown

        template<ExecutionStatus Status>
        inline constexpr bool Is() {
            return this->status == Status;
        }

        inline constexpr bool IsInvalidOrThrown() {
            return this->Is<ExecutionStatus::Invalid>() || this->Is<ExecutionStatus::ThrowableThrown>();
        }

        static inline ExecutionResult Void() {
            return { ExecutionStatus::VoidReturn, nullptr };
        }

        static inline ExecutionResult ReturnVariable(Ptr<Variable> var) {
            return { ExecutionStatus::VariableReturn, var };
        }

        static inline ExecutionResult Throw(Ptr<Variable> throwable_var) {
            return { ExecutionStatus::ThrowableThrown, throwable_var };
        }

        static inline ExecutionResult InvalidState() {
            return { ExecutionStatus::Invalid, nullptr };
        }

        static inline ExecutionResult ContinueCodeExecution() {
            return { ExecutionStatus::ContinueExecution, nullptr };
        }

    };

    namespace inner_impl {

        template<typename ...JArgs>
        ExecutionResult ExecuteStaticCode(u8 *code_ptr, u16 max_locals, ConstantPool pool, JArgs &&...java_args);

        template<typename ...JArgs>
        ExecutionResult ExecuteCode(u8 *code_ptr, u16 max_locals, Ptr<Variable> this_var, ConstantPool pool, JArgs &&...java_args);

        Ptr<ClassType> LocateClassTypeImpl(const std::string &class_name);

        inline Ptr<Variable> NewDefaultVariableImpl(VariableType type);

        Ptr<Variable> CreateNewString(const std::string &native_str);

        std::string GetStringValue(Ptr<Variable> str);

        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type);

        template<typename ...JArgs>
        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type, const std::string &init_descriptor, JArgs &&...java_args);

        ExecutionResult ThrowWithTypeImpl(const std::string &class_name);

        ExecutionResult ThrowWithTypeAndMessageImpl(const std::string &class_name, const std::string &msg);

    }

    // Separated from TypeUtils to solve potential circular dependencies :P

    struct ExtendedVariableType {
        VariableType type;
        Ptr<ClassType> class_type;

        static ExtendedVariableType MakeSimpleType(VariableType type) {
            return ExtendedVariableType { type, nullptr };
        }

        static ExtendedVariableType MakeClassType(const std::string &class_name) {
            return ExtendedVariableType { VariableType::ClassInstance, inner_impl::LocateClassTypeImpl(class_name) };
        }

        inline constexpr bool IsClassType() {
            return this->type == VariableType::ClassInstance;
        }

    };

    class TypeTraits {

        private:
            static inline std::map<VariableType, std::string> g_primitive_type_name_table =
            {
                { VariableType::Byte, "byte" },
                { VariableType::Boolean, "boolean" },
                { VariableType::Short, "short" },
                { VariableType::Character, "char" },
                { VariableType::Integer, "int" },
                { VariableType::Long, "long" },
                { VariableType::Float, "float" },
                { VariableType::Double, "double" },
            };

            static inline std::map<VariableType, std::string> g_primitive_type_table =
            {
                { VariableType::Byte, "B" },
                { VariableType::Boolean, "Z" },
                { VariableType::Short, "S" },
                { VariableType::Character, "C" },
                { VariableType::Integer, "I" },
                { VariableType::Long, "J" },
                { VariableType::Float, "F" },
                { VariableType::Double, "D" },
            };

        public:

            static bool IsPrimitiveType(const std::string &class_name) {
                for(auto &[type, name]: g_primitive_type_name_table) {
                    if(name == class_name) {
                        return true;
                    }
                }
                auto class_copy = class_name;
                while(class_copy.front() == '[') {
                    class_copy.erase(0, 1);
                }
                for(auto &[type, name]: g_primitive_type_table) {
                    if(name == class_copy) {
                        return true;
                    }
                }
                return false;
            }

            template<typename T>
            static inline constexpr VariableType DetermineVariableType() {

                #define _JAVM_DETERMINE_TYPE_BASE(type_name) \
                if constexpr(std::is_same_v<T, type::type_name>) { \
                    return VariableType::type_name; \
                }

                #define _JAVM_DETERMINE_TYPE_INTG_BASE(type_name) \
                if constexpr(std::is_same_v<T, type::type_name>) { \
                    return VariableType::Integer; \
                }

                _JAVM_DETERMINE_TYPE_INTG_BASE(Byte)
                _JAVM_DETERMINE_TYPE_INTG_BASE(Boolean)
                _JAVM_DETERMINE_TYPE_INTG_BASE(Short)
                _JAVM_DETERMINE_TYPE_INTG_BASE(Character)
                _JAVM_DETERMINE_TYPE_INTG_BASE(Integer)
                _JAVM_DETERMINE_TYPE_BASE(Long)
                _JAVM_DETERMINE_TYPE_BASE(Float)
                _JAVM_DETERMINE_TYPE_BASE(Double)
                _JAVM_DETERMINE_TYPE_BASE(ClassInstance)
                _JAVM_DETERMINE_TYPE_BASE(Array)

                #undef _JAVM_DETERMINE_TYPE_BASE
                #undef _JAVM_DETERMINE_TYPE_INTG_BASE

                return VariableType::Invalid;

            }

            template<typename T>
            static inline constexpr bool IsValidVariableType() {
                return DetermineVariableType<T>() != VariableType::Invalid;
            }

            static inline constexpr bool IsPrimitiveVariableType(VariableType type) {
                return (type != VariableType::Invalid) && (type != VariableType::ClassInstance) && (type != VariableType::Array) && (type != VariableType::NullObject);
            }

            static VariableType GetFieldNameType(const std::string &descriptor) {
                for(auto &[type, name] : g_primitive_type_name_table) {
                    if(name == descriptor) {
                        return type;
                    }
                }
                return VariableType::Invalid;
            }

            static VariableType GetFieldDescriptorType(const std::string &descriptor) {
                for(auto &[type, name] : g_primitive_type_table) {
                    if(name == descriptor) {
                        return type;
                    }
                }
                return VariableType::Invalid;
            }

            static inline VariableType GetFieldAnyType(const std::string &descriptor) {
                auto type = GetFieldNameType(descriptor);
                if(type == VariableType::Invalid) {
                    return GetFieldDescriptorType(descriptor);
                }
                return type;
            }

            static ExtendedVariableType GetFieldDescriptorFullType(const std::string &descriptor) {
                if(descriptor == "B") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Byte);
                }
                if(descriptor == "Z") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Boolean);
                }
                if(descriptor == "S") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Short);
                }
                if(descriptor == "C") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Character);
                }
                if(descriptor == "I") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Integer);
                }
                if(descriptor == "J") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Long);
                }
                if(descriptor == "F") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Float);
                }
                if(descriptor == "D") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Double);
                }
                if(descriptor.front() == '[') {
                    // array - TODO
                    return ExtendedVariableType::MakeSimpleType(VariableType::Array);
                }
                if(descriptor.front() == 'L') {
                    auto class_name = descriptor.substr(1, descriptor.length() - 2);
                    return ExtendedVariableType::MakeClassType(class_name);
                }
                return ExtendedVariableType::MakeSimpleType(VariableType::NullObject);
            }

            static std::string GetDescriptorForPrimitiveType(VariableType type) {
                if(g_primitive_type_table.find(type) != g_primitive_type_table.end()) {
                    return g_primitive_type_table[type];
                }
                return "";
            }

            static std::string GetNameForPrimitiveType(VariableType type) {
                if(g_primitive_type_name_table.find(type) != g_primitive_type_name_table.end()) {
                    return g_primitive_type_name_table[type];
                }
                return "";
            }

    };

    class ClassUtils {

        public:
            static std::string MakeSlashClassName(const std::string &input_name) {
                std::string copy = input_name;
                std::replace(copy.begin(), copy.end(), '.', '/');
                return copy;
            }

            static std::string MakeDotClassName(const std::string &input_name) {
                std::string copy = input_name;
                std::replace(copy.begin(), copy.end(), '/', '.');
                return copy;
            }

            static inline bool EqualClassNames(const std::string &name_a, const std::string &name_b) {
                // Directly convert both to slash names to avoid any trouble
                if(MakeSlashClassName(name_a) == MakeSlashClassName(name_b)) {
                    return true;
                }
                return false;
            }

    };

}