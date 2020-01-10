
#pragma once
#include <javm/javm_Base.hpp>
#include <functional>
#include <typeinfo>
#include <memory>

namespace javm::core {

    using DtorFunction = std::function<void(void*)>;
    using CloneFunction = std::function<void*(void*)>;

    void EmptyPointerDestructor(void*) {
    }

    template<typename T>
    void TypedPointerDestructor(void *ptr) {
        if(ptr != nullptr) {
            T *t_ptr = reinterpret_cast<T*>(ptr);
            delete t_ptr;
        }
    }

    void *EmptyPointerClone(void*) {
        return nullptr;
    }

    template<typename T>
    void *TypedPointerClone(void *ptr) {
        if(ptr != nullptr) {
            T *t_ptr = reinterpret_cast<T*>(ptr);
            T *clone_ptr = new T(*t_ptr);
            return reinterpret_cast<void*>(clone_ptr);
        }
        return nullptr;
    }

    enum class ValueType {

        Invalid,
        Void,
        Null,
        Byte,
        Boolean,
        Short,
        Character,
        Integer,
        Long,
        Float,
        Double,
        Array,
        ClassObject

    };

    class ClassObject;

    // Used to create void values - anyway, any value not matching the list above will be considered as void :P
    struct VoidValue {};

    // Invalid values are the values returned in any unexpected case (bad calls, invalid types/variables/classes...)
    struct InvalidValue {};
    
    class Array;

    class ValuePointerHolder {

        private:
            void *inner_ptr;
            size_t type_hash_code;
            DtorFunction dtor;
            CloneFunction clone;
            ValueType type;

        public:
            ValuePointerHolder(void *t, size_t type_hash, ValueType vtype, CloneFunction clone_fn = EmptyPointerClone, DtorFunction dtor_fn = EmptyPointerDestructor) : inner_ptr(t), type_hash_code(type_hash), dtor(dtor_fn), clone(clone_fn), type(vtype) {}
            ValuePointerHolder() : inner_ptr(nullptr), type_hash_code(0), type(ValueType::Null),  dtor(EmptyPointerDestructor), clone(EmptyPointerClone) {}

            ~ValuePointerHolder() {
                this->Dispose();
            }

            void Dispose() {
                if(this->inner_ptr != nullptr) {
                    this->dtor(this->inner_ptr); // if the value is empty, this function is the empty destructor
                    this->inner_ptr = nullptr;
                }
            }

            template<typename T>
            static constexpr ValueType DetectValueType() {
                if constexpr(std::is_same_v<T, int>) {
                    return ValueType::Integer;
                }
                if constexpr(std::is_same_v<T, long>) {
                    return ValueType::Long;
                }
                if constexpr(std::is_same_v<T, float>) {
                    return ValueType::Float;
                }
                if constexpr(std::is_same_v<T, double>) {
                    return ValueType::Double;
                }
                if constexpr(std::is_same_v<T, bool>) {
                    return ValueType::Boolean;
                }
                if constexpr(std::is_same_v<T, char>) {
                    return ValueType::Character;
                }
                if constexpr(std::is_same_v<T, nullptr_t>) {
                    return ValueType::Null;
                }
                if constexpr(std::is_same_v<T, Array>) {
                    return ValueType::Array;
                }
                if constexpr(std::is_same_v<T, VoidValue>) {
                    return ValueType::Void;
                }
                if constexpr(std::is_same_v<T, ClassObject> || std::is_base_of_v<ClassObject, T>) {
                    return ValueType::ClassObject;
                }
                return ValueType::Invalid;
            }

            template<typename T>
            static constexpr bool IsValidValueType() {
                return DetectValueType<T>() != ValueType::Void;
            }

            template<typename T>
            bool IsValidCast() {
                return this->type_hash_code == typeid(T).hash_code();
            }

            size_t GetTypeHashCode() {
                return this->type_hash_code;
            }

            bool IsValid() {
                return this->type != ValueType::Invalid;
            }

            bool IsInvalid() {
                return !this->IsValid();
            }

            bool IsNull() {
                return this->type == ValueType::Null;
            }

            bool IsVoid() {
                return this->type == ValueType::Void;
            }

            bool IsClassObject() {
                return this->type == ValueType::ClassObject;
            }

            bool IsArray() {
                return this->type == ValueType::Array;
            }
            
            ValueType GetValueType() {
                return this->type;
            }

            DtorFunction GetDtorFunction() {
                return this->dtor;
            }

            CloneFunction GetCloneFunction() {
                return this->clone;
            }

            ValuePointerHolder Clone() {
                auto cloned_ptr = this->clone(this->inner_ptr);
                return ValuePointerHolder(cloned_ptr, this->type_hash_code, this->type, this->clone, this->dtor);
            }

            void *GetAddress() {
                return this->inner_ptr;
            }

            template<typename T>
            T *GetReference() {
                if(this->IsNull()) {
                    return nullptr;
                }
                return reinterpret_cast<T*>(this->inner_ptr);
            }

            template<typename T>
            T Get() {
                auto ptr = this->GetReference<T>();
                if(ptr == nullptr) {
                    return T();
                }
                return *ptr;
            }

            template<typename T>
            void Set(T value) {
                auto ptr = this->GetReference<T>();
                if(ptr != nullptr) {
                    memcpy(ptr, &value, sizeof(value));
                }
            }
    };

    using Value = std::shared_ptr<ValuePointerHolder>;

    template<typename T, typename ...Args>
    static inline Value CreateNewValue(Args &&...args) {
        T *t_ptr = new T(args...);
        return std::make_shared<ValuePointerHolder>(reinterpret_cast<void*>(t_ptr), typeid(T).hash_code(), ValuePointerHolder::DetectValueType<T>(), TypedPointerClone<T>, TypedPointerDestructor<T>);
    }

    template<typename T>
    static inline Value CreateExistingValue(T *t) {
        return std::make_shared<ValuePointerHolder>(reinterpret_cast<void*>(t), typeid(T).hash_code(), ValuePointerHolder::DetectValueType<T>(), TypedPointerClone<T>);
    }

    static inline Value CreateVoidValue() {
        return CreateNewValue<VoidValue>();
    }

    static inline Value CreateInvalidValue() {
        return CreateNewValue<InvalidValue>();
    }

    static inline Value CreateNullValue() {
        return std::make_shared<ValuePointerHolder>();
    }

    static inline Value CloneValue(Value val) {
        return std::make_shared<ValuePointerHolder>(val->Clone());
    }

    #define JAVM_ASSERT_VALID_VALUE(ptr, val, what, ...) { \
        if(val->IsInvalid()) { \
            (ptr)->ThrowWithType("java.lang.RuntimeException", what); \
        } \
        else { \
            __VA_ARGS__ \
        } \
    }

    #define JAVM_ASSERT_VALID_VALUE_VAR(var, val, what, ...) JAVM_ASSERT_VALID_VALUE(&var, val, what, ##__VA_ARGS__)

    class Array {

        private:
            ValueType type;
            u32 array_length;
            std::vector<Value> value_list;

        public:
            Array(ValueType value_type, u32 length) : type(value_type), array_length(length) {
                for(u32 i = 0; i < length; i++) {
                    value_list.emplace_back();
                }
            }

            u32 GetLength() {
                return this->array_length;
            }

            ValueType GetValueType() {
                return this->type;
            }

            bool CheckIndex(u32 index) {
                return index < this->GetLength();
            }

            Value GetAt(u32 index) {
                if(this->CheckIndex(index)) {
                    return this->value_list.at(index);
                }
                return CreateInvalidValue();
            }

            void SetAt(u32 index, Value val) {
                if(this->CheckIndex(index)) {
                    this->value_list.at(index) = val;
                }
            }
    };
    
    template<typename T>
    Value CreateArray(u32 length) {
        static_assert(ValuePointerHolder::IsValidValueType<T>(), "Invalid value type");
        return CreateNewValue<Array>(ValuePointerHolder::DetectValueType<T>(), length);
    }
    
    template<typename T>
    Value CreateArray(std::initializer_list<T> list) {
        static_assert(ValuePointerHolder::IsValidValueType<T>(), "Invalid value type");
        auto array_val = CreateNewValue<Array>(ValuePointerHolder::DetectValueType<T>(), list.size());
        auto array_ref = array_val->template GetReference<Array>();
        u32 i = 0;
        for(auto &item: list) {
            if(!array_ref->CheckIndex(i)) {
                break;
            }
            array_ref->SetAt(i, CreateNewValue<T>(item));
            i++;
        }
        return array_val;
    }
}