
#pragma once
#include <javm/vm/vm_Array.hpp>
#include <javm/vm/ref/ref_Reflection.hpp>

namespace javm::vm {

    class Variable {
        private:
            union VariableValue {
                Ptr<type::Integer> common_int_val;
                Ptr<type::Long> long_val;
                Ptr<type::Float> float_val;
                Ptr<type::Double> double_val;
                Ptr<type::ClassInstance> class_val;
                Ptr<type::Array> arr_val;
                Ptr<type::NullObject> null_val;

                VariableValue(Ptr<type::Integer> common_int_val) : common_int_val(common_int_val) {}
                VariableValue(Ptr<type::Long> long_val) : long_val(long_val) {}
                VariableValue(Ptr<type::Float> float_val) : float_val(float_val) {}
                VariableValue(Ptr<type::Double> double_val) : double_val(double_val) {}
                VariableValue(Ptr<type::ClassInstance> class_val) : class_val(class_val) {}
                VariableValue(Ptr<type::Array> arr_val) : arr_val(arr_val) {}
                VariableValue(Ptr<type::NullObject> null_val) : null_val(null_val) {}

                ~VariableValue() {}

                template<typename T>
                inline Ptr<T> Get() {
                    if constexpr(std::is_same_v<T, type::Integer>) {
                        return this->common_int_val;
                    }
                    else if constexpr(std::is_same_v<T, type::Long>) {
                        return this->long_val;
                    }
                    else if constexpr(std::is_same_v<T, type::Float>) {
                        return this->float_val;
                    }
                    else if constexpr(std::is_same_v<T, type::Double>) {
                        return this->double_val;
                    }
                    else if constexpr(std::is_same_v<T, type::ClassInstance>) {
                        return this->class_val;
                    }
                    else if constexpr(std::is_same_v<T, type::Array>) {
                        return this->arr_val;
                    }
                    else if constexpr(std::is_same_v<T, type::NullObject>) {
                        return this->null_val;
                    }
                    else {
                        return nullptr;
                    }
                }

                template<typename T>
                inline void Set(Ptr<T> val) {
                    if constexpr(std::is_same_v<T, type::Integer>) {
                        this->common_int_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::Long>) {
                        this->long_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::Float>) {
                        this->float_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::Double>) {
                        this->double_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::ClassInstance>) {
                        this->class_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::Array>) {
                        this->arr_val = val;
                    }
                    else if constexpr(std::is_same_v<T, type::NullObject>) {
                        this->null_val = val;
                    }
                }
            };

            VariableType type;
            VariableValue value;

        public:
            #define _JAVM_VAR_CTOR(type_name) Variable(Ptr<type::type_name> val) : type(VariableType::type_name), value(val) {}

            _JAVM_VAR_CTOR(Integer) // Byte, Boolean, Character and Short are also handled here
            _JAVM_VAR_CTOR(Long)
            _JAVM_VAR_CTOR(Float)
            _JAVM_VAR_CTOR(Double)
            _JAVM_VAR_CTOR(ClassInstance)
            _JAVM_VAR_CTOR(Array)
            _JAVM_VAR_CTOR(NullObject)

            #undef _JAVM_VAR_CTOR

            template<VariableType Type>
            inline constexpr bool CanGetAs() {
                return this->type == Type;
            }

            inline constexpr bool IsBigComputationalType() {
                return this->CanGetAs<VariableType::Long>() || this->CanGetAs<VariableType::Double>();
            }

            inline VariableType GetType() {
                return this->type;
            }

            inline constexpr bool IsNull() {
                return this->type == VariableType::NullObject;
            }

            template<typename T>
            inline Ptr<T> GetAs() {
                static_assert(IsValidVariableType<T>(), "Invalid type");
                constexpr auto v_type = DetermineVariableType<T>();
                if(v_type != this->type) {
                    // TODO: critical error!
                    return nullptr;
                }
                
                return this->value.Get<T>();
            }

            template<typename T>
            inline T GetValue() {
                auto obj = this->GetAs<T>();
                return ptr::GetValue(obj);
            }

            template<typename T>
            inline void SetAs(Ptr<T> val) {
                static_assert(IsValidVariableType<T>(), "Invalid type");
                const auto v_type = DetermineVariableType<T>();
                if(v_type != this->type) {
                    // TODO: critical error!
                    return;
                }

                this->value.Set(val);
            }
    };

    template<typename T>
    inline Ptr<Variable> NewPrimitiveVariable(const T t) {
        static_assert(IsPrimitiveType<T>(), "Invalid primitive type");

        return ptr::New<Variable>(ptr::New<T>(t));
    }

    inline Ptr<Variable> NewDefaultPrimitiveVariable(const VariableType type) {
        if(!IsPrimitiveVariableType(type)) {
            return nullptr;
        }

        #define _JAVM_DEFAULT_VALUE_IMPL(type_name, val) \
        if(type == VariableType::type_name) { \
            return NewPrimitiveVariable(val); \
        }

        _JAVM_DEFAULT_VALUE_IMPL(Byte, static_cast<type::Byte>(0))
        _JAVM_DEFAULT_VALUE_IMPL(Boolean, static_cast<type::Boolean>(false))
        _JAVM_DEFAULT_VALUE_IMPL(Short, static_cast<type::Short>(0))
        _JAVM_DEFAULT_VALUE_IMPL(Character, static_cast<type::Character>(u'\0'))
        _JAVM_DEFAULT_VALUE_IMPL(Integer, static_cast<type::Integer>(0))
        _JAVM_DEFAULT_VALUE_IMPL(Long, static_cast<type::Long>(0))
        _JAVM_DEFAULT_VALUE_IMPL(Float, static_cast<type::Float>(0.0f))
        _JAVM_DEFAULT_VALUE_IMPL(Double, static_cast<type::Double>(0.0f))

        #undef _JAVM_DEFAULT_VALUE_IMPL

        // TODO: is this even reachable?
        return nullptr;
    }

    inline Ptr<Variable> MakeNull() {
        return ptr::New<Variable>(ptr::New<type::NullObject>());
    }
    
    template<typename T>
    inline Ptr<Variable> NewDefaultPrimitiveVariable() {
        static_assert(IsPrimitiveType<T>(), "Invalid primitive type");

        return NewDefaultPrimitiveVariable(DetermineVariableType<T>());
    }

    inline Ptr<Variable> NewDefaultVariable(const VariableType type) {
        if(type == VariableType::Invalid) {
            return nullptr;
        }
        else if(type == VariableType::ClassInstance) {
            return MakeNull();
        }
        else if(type == VariableType::Array) {
            return MakeNull();
        }
        else {
            return NewDefaultPrimitiveVariable(type);
        }
    }

    inline Ptr<Variable> NewClassVariable(Ptr<ClassType> class_type) {
        return ptr::New<Variable>(ptr::New<type::ClassInstance>(class_type));
    }

    template<typename ...JArgs>
    inline Ptr<Variable> NewClassVariable(Ptr<ClassType> class_type, const String &init_descriptor, JArgs &&...java_args) {
        auto class_var = ptr::New<Variable>(ptr::New<type::ClassInstance>(class_type));
        
        auto class_obj = class_var->GetAs<type::ClassInstance>();
        class_obj->CallConstructor(class_var, init_descriptor, java_args...);

        return class_var;
    }

    inline Ptr<Variable> NewArrayVariable(const u32 length, const VariableType type, const u32 dimension = 1) {
        return ptr::New<Variable>(ptr::New<type::Array>(type, length, dimension));
    }

    inline Ptr<Variable> NewArrayVariable(const u32 length, Ptr<ClassType> type, const u32 dimension = 1) {
        return ptr::New<Variable>(ptr::New<type::Array>(type, length, dimension));
    }

    template<typename ...JArgs>
    inline Ptr<Variable> NewArray(VariableType type, JArgs &&...java_args) {
        auto arr_obj = ptr::New<type::Array>(type, sizeof...(JArgs));

        u32 idx = 0;
        (arr_obj->SetAt(idx++, java_args), ...);

        return ptr::New<Variable>(arr_obj);
    }

    // TODO: easy support for creating and using multi-dimensional arrays from C++?

    inline Ptr<Variable> MakeTrue() {
        return NewPrimitiveVariable<type::Boolean>(true);
    }

    inline Ptr<Variable> MakeFalse() {
        return NewPrimitiveVariable<type::Boolean>(false);
    }

    // New java.lang.Class variable from reflection type

    Ptr<Variable> NewClassTypeVariable(Ptr<ref::ReflectionType> ref_type);

    String FormatVariableType(Ptr<Variable> var);
    String FormatVariable(Ptr<Variable> var);

}