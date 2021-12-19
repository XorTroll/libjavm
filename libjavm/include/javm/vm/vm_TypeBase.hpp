
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
        // (All the first 5 types below are basically treated as 32-bit signed integers)

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
        Thrown,
    };

    struct ExecutionResult {
        ExecutionStatus status;
        Ptr<Variable> ret_var; // nullptr if void return, variable if var returned, throwable var if thrown

        template<ExecutionStatus Status>
        inline constexpr bool Is() const {
            return this->status == Status;
        }

        inline constexpr bool IsInvalidOrThrown() const {
            return this->Is<ExecutionStatus::Invalid>() || this->Is<ExecutionStatus::Thrown>();
        }

        static inline ExecutionResult Void() {
            return { ExecutionStatus::VoidReturn, nullptr };
        }

        static inline ExecutionResult ReturnVariable(Ptr<Variable> var) {
            return { ExecutionStatus::VariableReturn, var };
        }

        static inline ExecutionResult Thrown() {
            return { ExecutionStatus::Thrown, nullptr };
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
        ExecutionResult ExecuteStaticCode(const u8 *code_ptr, const u16 max_locals, ConstantPool pool, JArgs &&...java_args);

        template<typename ...JArgs>
        ExecutionResult ExecuteCode(const u8 *code_ptr, const u16 max_locals, Ptr<Variable> this_var, ConstantPool pool, JArgs &&...java_args);

        Ptr<ClassType> LocateClassTypeImpl(const String &class_name);

        inline Ptr<Variable> NewDefaultVariableImpl(const VariableType type);

        Ptr<Variable> CreateNewString(const String &native_str);

        String GetStringValue(Ptr<Variable> str);

        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type);

        template<typename ...JArgs>
        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type, const String &init_descriptor, JArgs &&...java_args);

        ExecutionResult ThrowWithTypeImpl(const String &class_name);

        ExecutionResult ThrowWithTypeAndMessageImpl(const String &class_name, const String &msg);

        static inline bool WasExceptionThrownImpl();

        static inline void NotifyExceptionThrownImpl(Ptr<Variable> throwable_v);

    }

    // Separated from TypeUtils to solve potential circular dependencies

    struct ExtendedVariableType {
        VariableType type;
        Ptr<ClassType> class_type;

        static ExtendedVariableType MakeSimpleType(const VariableType type) {
            return ExtendedVariableType { type, nullptr };
        }

        static ExtendedVariableType MakeClassType(const String &class_name) {
            return ExtendedVariableType { VariableType::ClassInstance, inner_impl::LocateClassTypeImpl(class_name) };
        }

        inline constexpr bool IsClassType() {
            return this->type == VariableType::ClassInstance;
        }
    };

    class TypeTraits {
        private:
            static const inline std::map<VariableType, String> PrimitiveTypeNameTable = {
                { VariableType::Byte, u"byte" },
                { VariableType::Boolean, u"boolean" },
                { VariableType::Short, u"short" },
                { VariableType::Character, u"char" },
                { VariableType::Integer, u"int" },
                { VariableType::Long, u"long" },
                { VariableType::Float, u"float" },
                { VariableType::Double, u"double" },
            };

            static const inline std::map<VariableType, String> PrimitiveTypeDescriptorTable = {
                { VariableType::Byte, u"B" },
                { VariableType::Boolean, u"Z" },
                { VariableType::Short, u"S" },
                { VariableType::Character, u"C" },
                { VariableType::Integer, u"I" },
                { VariableType::Long, u"J" },
                { VariableType::Float, u"F" },
                { VariableType::Double, u"D" },
            };

        public:
            static bool IsPrimitiveType(const String &class_name) {
                for(auto &[type, name]: PrimitiveTypeNameTable) {
                    if(name == class_name) {
                        return true;
                    }
                }

                auto class_copy = class_name;
                while(class_copy.front() == u'[') {
                    class_copy.erase(0, 1);
                }
                for(auto &[type, name]: PrimitiveTypeDescriptorTable) {
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

            static VariableType GetFieldNameType(const String &descriptor) {
                for(auto &[type, name] : PrimitiveTypeNameTable) {
                    if(name == descriptor) {
                        return type;
                    }
                }
                return VariableType::Invalid;
            }

            static VariableType GetFieldDescriptorType(const String &descriptor) {
                for(auto &[type, name] : PrimitiveTypeDescriptorTable) {
                    if(name == descriptor) {
                        return type;
                    }
                }
                return VariableType::Invalid;
            }

            static inline VariableType GetFieldAnyType(const String &descriptor) {
                auto type = GetFieldNameType(descriptor);
                if(type == VariableType::Invalid) {
                    return GetFieldDescriptorType(descriptor);
                }
                return type;
            }

            static ExtendedVariableType GetFieldDescriptorFullType(const String &descriptor) {
                if(descriptor == u"B") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Byte);
                }
                if(descriptor == u"Z") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Boolean);
                }
                if(descriptor == u"S") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Short);
                }
                if(descriptor == u"C") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Character);
                }
                if(descriptor == u"I") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Integer);
                }
                if(descriptor == u"J") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Long);
                }
                if(descriptor == u"F") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Float);
                }
                if(descriptor == u"D") {
                    return ExtendedVariableType::MakeSimpleType(VariableType::Double);
                }
                if(descriptor.front() == u'[') {
                    // array - TODO
                    return ExtendedVariableType::MakeSimpleType(VariableType::Array);
                }
                if(descriptor.front() == u'L') {
                    auto class_name = descriptor.substr(1, descriptor.length() - 2);
                    return ExtendedVariableType::MakeClassType(class_name);
                }
                return ExtendedVariableType::MakeSimpleType(VariableType::NullObject);
            }

            static String GetDescriptorForPrimitiveType(VariableType type) {
                if(PrimitiveTypeDescriptorTable.find(type) != PrimitiveTypeDescriptorTable.end()) {
                    return PrimitiveTypeDescriptorTable.at(type);
                }

                // TODO
                return u"<no-desc>";
            }

            static String GetNameForPrimitiveType(VariableType type) {
                if(PrimitiveTypeNameTable.find(type) != PrimitiveTypeNameTable.end()) {
                    return PrimitiveTypeNameTable.at(type);
                }

                // TODO
                return u"<no-name>";
            }
    };

    class ClassUtils {
        public:
            static String MakeSlashClassName(const String &input_name) {
                auto copy = input_name;
                std::replace(copy.begin(), copy.end(), u'.', u'/');
                return copy;
            }

            static String MakeDotClassName(const String &input_name) {
                auto copy = input_name;
                std::replace(copy.begin(), copy.end(), u'/', u'.');
                return copy;
            }

            static inline bool EqualClassNames(const String &name_a, const String &name_b) {
                // Directly convert both to slash names to avoid any trouble
                if(MakeSlashClassName(name_a) == MakeSlashClassName(name_b)) {
                    return true;
                }
                return false;
            }
    };

}