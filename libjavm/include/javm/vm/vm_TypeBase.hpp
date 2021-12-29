
#pragma once
#include <javm/vm/vm_ConstantPool.hpp>
#include <map>

namespace javm::vm {

    class ClassType;
    class ClassInstance;
    class Variable;
    class Array;
    struct ExceptionTableEntry;

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
        Thrown
    };

    struct ExecutionResult {
        ExecutionStatus status;
        bool catchable_throw;
        Ptr<Variable> var; // nullptr if void return, variable if var returned, throwable var if thrown

        template<ExecutionStatus Status>
        inline constexpr bool Is() const {
            return this->status == Status;
        }

        inline constexpr bool IsInvalidOrThrown() const {
            return this->Is<ExecutionStatus::Invalid>() || this->Is<ExecutionStatus::Thrown>();
        }

        static inline ExecutionResult Void() {
            return { ExecutionStatus::VoidReturn, false, nullptr };
        }

        static inline ExecutionResult ReturnVariable(Ptr<Variable> var) {
            return { ExecutionStatus::VariableReturn, false, var };
        }

        static inline ExecutionResult Throw(Ptr<Variable> throwable, const bool is_catchable = true) {
            return { ExecutionStatus::Thrown, is_catchable, throwable };
        }

        static inline ExecutionResult InvalidState() {
            return { ExecutionStatus::Invalid, false, nullptr };
        }

        static inline ExecutionResult ContinueCodeExecution() {
            return { ExecutionStatus::ContinueExecution, false, nullptr };
        }

    };

    bool IsPrimitiveType(const String &type_name);

    String GetPrimitiveTypeDescriptor(const VariableType type);
    String GetPrimitiveTypeName(const VariableType type);

    VariableType GetPrimitiveVariableTypeByName(const String &type_name);
    VariableType GetPrimitiveVariableTypeByDescriptor(const String &type_descriptor);
    VariableType GetVariableTypeByDescriptor(const String &type_descriptor);

    inline String GetClassNameFromDescriptor(const String &class_descriptor) {
        auto class_desc_copy = class_descriptor;
        
        while(class_desc_copy.front() == u'[') {
            class_desc_copy.erase(0, 1);
        }
        while(class_desc_copy.front() == u'L') {
            class_desc_copy.erase(0, 1);
        }
        if(class_desc_copy.back() == u';') {
            class_desc_copy.pop_back();
        }

        return class_desc_copy;
    }
    
    inline String MakeSlashClassName(const String &input_name) {
        auto copy = input_name;
        std::replace(copy.begin(), copy.end(), u'.', u'/');
        return copy;
    }

    inline String MakeDotClassName(const String &input_name) {
        auto copy = input_name;
        std::replace(copy.begin(), copy.end(), u'/', u'.');
        return copy;
    }

    inline bool EqualClassNames(const String &name_a, const String &name_b) {
        // Directly convert both to slash names to avoid any trouble
        return MakeSlashClassName(name_a) == MakeSlashClassName(name_b);
    }

    template<typename T>
    inline constexpr VariableType DetermineVariableType() {
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
    inline constexpr bool IsValidVariableType() {
        return DetermineVariableType<T>() != VariableType::Invalid;
    }

    inline constexpr bool IsPrimitiveVariableType(const VariableType type) {
        return (type != VariableType::Invalid) && (type != VariableType::ClassInstance) && (type != VariableType::Array) && (type != VariableType::NullObject);
    }

    template<typename T>
    inline constexpr bool IsPrimitiveType() {
        return IsPrimitiveVariableType(DetermineVariableType<T>());
    }

    inline constexpr bool IsCommonIntegerVariableType(const VariableType type) {
        return (type == VariableType::Byte) || (type == VariableType::Boolean) || (type == VariableType::Short) || (type == VariableType::Character) || (type == VariableType::Integer);
    }

    inline constexpr bool IsVariableTypeConvertibleTo(const VariableType src_type, const VariableType dst_type) {
        if(IsCommonIntegerVariableType(src_type) && IsCommonIntegerVariableType(dst_type)) {
            return true;
        }

        if((src_type == VariableType::NullObject) == (dst_type == VariableType::ClassInstance)) {
            return true;
        }
        if((src_type == VariableType::Array) == (dst_type == VariableType::ClassInstance)) {
            return true;
        }

        return src_type == dst_type;
    }

}