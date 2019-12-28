
#pragma once
#include <javm/javm_Base.hpp>
#include <functional>
#include <typeinfo>

namespace javm::core {

    using DtorFunction = std::function<void(void*)>;

    void EmptyPointerDestructor(void*) {
    }

    template<typename T>
    void TypeDeletePointerDestructor(void *ptr) {
        if(ptr != nullptr) {
            T *t_ptr = reinterpret_cast<T*>(ptr);
            delete t_ptr;
        }
    }

    enum class ValueType {

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

    class ValuePointerHolder {

        private:
            void *inner_ptr;
            size_t type_hash_code;
            DtorFunction dtor;
            ValueType type;

        public:
            ValuePointerHolder(void *t, size_t type_hash, ValueType vtype, DtorFunction dtor_fn = EmptyPointerDestructor) : inner_ptr(t), type_hash_code(type_hash), type(vtype), dtor(dtor_fn) {}
            ValuePointerHolder() : inner_ptr(nullptr), type_hash_code(0), type(ValueType::Null), dtor(EmptyPointerDestructor) {}
            ValuePointerHolder(const ValuePointerHolder &other) : inner_ptr(other.inner_ptr), type_hash_code(other.type_hash_code), dtor(other.dtor), type(other.type) {}

            ValuePointerHolder operator=(ValuePointerHolder other) {
                this->inner_ptr = other.GetAddress();
                this->type_hash_code = other.GetTypeHashCode();
                this->dtor = other.GetDtorFunction();
                this->type = other.GetValueType();
                return *this;
            }

            void Dispose() {
                if(this->inner_ptr != nullptr) {
                    this->dtor(this->inner_ptr); // if the holder is empty, this function is the empty destructor
                    this->inner_ptr = nullptr;
                }
            }

            template<typename T>
            static ValueType DetectValueType() {
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
                if constexpr(std::is_same_v<T, std::vector<ValuePointerHolder>>) {
                    return ValueType::Array;
                }
                if constexpr(std::is_same_v<T, ClassObject> || std::is_base_of_v<ClassObject, T>) {
                    return ValueType::ClassObject;
                }
                return ValueType::Void;
            }

            template<typename T>
            bool IsValidCast() {
                return this->type_hash_code == typeid(T).hash_code();
            }

            size_t GetTypeHashCode() {
                return this->type_hash_code;
            }

            template<typename T, typename ...Args>
            static inline ValuePointerHolder Create(Args &&...args) {
                T *t_ptr = new T(args...);
                return ValuePointerHolder(reinterpret_cast<void*>(t_ptr), typeid(T).hash_code(), DetectValueType<T>(), TypeDeletePointerDestructor<T>);
            }

            template<typename T>
            static ValuePointerHolder CreateFromExisting(T *t) {
                return ValuePointerHolder(reinterpret_cast<void*>(t), typeid(T).hash_code(), DetectValueType<T>());
            }

            static ValuePointerHolder CreateVoid() {
                return Create<VoidValue>();
            }

            static ValuePointerHolder CreateNull() {
                return ValuePointerHolder();
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
            
            ValueType GetValueType() {
                return this->type;
            }

            DtorFunction GetDtorFunction() {
                return this->dtor;
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

    using Array = std::vector<ValuePointerHolder>;

    template<typename T>
    Array CreateArray(std::initializer_list<T> list) {
        Array array;
        for(auto &item: list) {
            array.push_back(ValuePointerHolder::Create<T>(item));
        }
        return array;
    }
}