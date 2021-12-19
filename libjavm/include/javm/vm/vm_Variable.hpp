
#pragma once
#include <javm/vm/vm_Type.hpp>
#include <javm/vm/vm_Reflection.hpp>

namespace javm::vm {

    class Variable {

        private:
            VariableType type;
            Ptr<type::Integer> common_int_val;
            Ptr<type::Long> long_val;
            Ptr<type::Float> float_val;
            Ptr<type::Double> double_val;
            Ptr<type::ClassInstance> class_val;
            Ptr<type::Array> arr_val;
            Ptr<type::NullObject> null_val;

        public:

            #define _JAVM_VAR_CTOR(type_name, val_name) Variable(Ptr<type::type_name> val) : type(VariableType::type_name), val_name(val) {}

            _JAVM_VAR_CTOR(Integer, common_int_val)
            _JAVM_VAR_CTOR(Long, long_val)
            _JAVM_VAR_CTOR(Float, float_val)
            _JAVM_VAR_CTOR(Double, double_val)
            _JAVM_VAR_CTOR(ClassInstance, class_val)
            _JAVM_VAR_CTOR(Array, arr_val)
            _JAVM_VAR_CTOR(NullObject, null_val)

            template<VariableType Type>
            inline constexpr bool CanGetAs() {
                return this->type == Type;
            }

            inline constexpr bool IsBigComputationalType() {
                return this->CanGetAs<VariableType::Long>() || this->CanGetAs<VariableType::Double>();
            }

            VariableType GetType() {
                return this->type;
            }

            inline constexpr bool IsNull() {
                return this->type == VariableType::NullObject;
            }

            template<typename T>
            Ptr<T> GetAs() {
                static_assert(TypeTraits::IsValidVariableType<T>(), "Invalid type");
                constexpr auto v_type = TypeTraits::DetermineVariableType<T>();
                
                if constexpr(v_type == VariableType::Integer) {
                    return this->common_int_val;
                }
                if constexpr(v_type == VariableType::Long) {
                    return this->long_val;
                }
                if constexpr(v_type == VariableType::Float) {
                    return this->float_val;
                }
                if constexpr(v_type == VariableType::Double) {
                    return this->double_val;
                }
                if constexpr(v_type == VariableType::ClassInstance) {
                    return this->class_val;
                }
                if constexpr(v_type == VariableType::Array) {
                    return this->arr_val;
                }
                if constexpr(v_type == VariableType::NullObject) {
                    return this->null_val;
                }
                return nullptr;
            }

            template<typename T>
            T GetValue() {
                auto obj = this->GetAs<T>();
                return ptr::GetValue(obj);
            }

            template<typename T>
            void SetAs(Ptr<T> val) {
                static_assert(TypeTraits::IsValidVariableType<T>(), "Invalid type");
                const auto v_type = TypeTraits::DetermineVariableType<T>();
                if(v_type != this->type) {
                    return;
                }
                if constexpr(v_type == VariableType::Integer) {
                    this->common_int_val = val;
                }
                if constexpr(v_type == VariableType::Long) {
                    this->long_val = val;
                }
                if constexpr(v_type == VariableType::Float) {
                    this->float_val = val;
                }
                if constexpr(v_type == VariableType::Double) {
                    this->double_val = val;
                }
                if constexpr(v_type == VariableType::ClassInstance) {
                    this->class_val = val;
                }
                if constexpr(v_type == VariableType::Array) {
                    this->arr_val = val;
                }
                if constexpr(v_type == VariableType::NullObject) {
                    this->null_val = val;
                }
            }

    };

    class TypeUtils {

        private:
            static inline Ptr<Variable> g_null_ref_var;
            static inline std::vector<Ptr<Variable>> g_cached_class_types;

            static inline void SetInArray(Ptr<Variable> var, Ptr<Array> &array, u32 &idx) {
                array->SetAt(idx, var);
                idx++;
            }

            static inline Ptr<Variable> GetNullVariableImpl() {
                if(g_null_ref_var) {
                    return g_null_ref_var;
                }
                g_null_ref_var = ptr::New<Variable>(ptr::New<NullObject>());
                return g_null_ref_var;
            }

            static Ptr<Variable> GetCachedClassType(const String &class_name) {
                for(auto &class_v: g_cached_class_types) {
                    auto class_obj = class_v->GetAs<type::ClassInstance>();
                    auto name_v = class_obj->GetField(u"name", u"Ljava/lang/String;");
                    auto name = inner_impl::GetStringValue(name_v);
                    if(class_name == name) {
                        return class_v;
                    }
                }
                return nullptr;
            }

            static inline void CacheClassType(Ptr<Variable> class_v) {
                g_cached_class_types.push_back(class_v);
            }

        public:
            static inline bool IsPrimitiveType(VariableType type) {
                if(type != VariableType::Invalid) {
                    if(type != VariableType::ClassInstance) {
                        if(type != VariableType::Array) {
                            return true;
                        }
                    }
                }
                return false;
            }
            
            // Variable creation

            template<typename T>
            static inline Ptr<Variable> NewPrimitiveVariable(T t) {
                static_assert(TypeTraits::IsValidVariableType<T>(), "Invalid type");
                // Primitive implies no class or array
                static_assert(!std::is_same_v<T, type::ClassInstance>, "Invalid type");
                static_assert(!std::is_same_v<T, type::Array>, "Invalid type");

                return ptr::New<Variable>(ptr::New<T>(t));
            }

            template<typename T>
            static inline Ptr<Variable> NewDefaultVariable() {
                static_assert(TypeTraits::IsValidVariableType<T>(), "Invalid type");
                // Primitive implies no class or array
                static_assert(!std::is_same_v<T, type::ClassInstance>, "Invalid type");
                static_assert(!std::is_same_v<T, type::Array>, "Invalid type");

                #define _JAVM_DEFAULT_VALUE_IMPL(type_name, val) \
                if constexpr(std::is_same_v<T, type::type_name>) { \
                    return ptr::New<Variable>(ptr::New<T>(val)); \
                }

                _JAVM_DEFAULT_VALUE_IMPL(Byte, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Boolean, false)
                _JAVM_DEFAULT_VALUE_IMPL(Short, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Character, '\0')
                _JAVM_DEFAULT_VALUE_IMPL(Integer, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Long, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Float, 0.0f)
                _JAVM_DEFAULT_VALUE_IMPL(Double, 0.0f)

                #undef _JAVM_DEFAULT_VALUE_IMPL

                // return null by default, which should be class object or array
                return Null();
            }

            static inline Ptr<Variable> NewDefaultVariable(VariableType type) {
                if(type == VariableType::Invalid) {
                    return nullptr;
                }
                if(type == VariableType::ClassInstance) {
                    return Null();
                }
                if(type == VariableType::Array) {
                    return Null();
                }

                #define _JAVM_DEFAULT_VALUE_IMPL(type_name, val) \
                if(type == VariableType::type_name) { \
                    return ptr::New<Variable>(ptr::New<type::type_name>(val)); \
                }

                _JAVM_DEFAULT_VALUE_IMPL(Byte, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Boolean, false)
                _JAVM_DEFAULT_VALUE_IMPL(Short, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Character, '\0')
                _JAVM_DEFAULT_VALUE_IMPL(Integer, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Long, 0)
                _JAVM_DEFAULT_VALUE_IMPL(Float, 0.0f)
                _JAVM_DEFAULT_VALUE_IMPL(Double, 0.0f)

                #undef _JAVM_DEFAULT_VALUE_IMPL

                return nullptr;
            }

            static Ptr<Variable> NewClassVariable(Ptr<ClassType> class_type) {
                auto class_var = ptr::New<Variable>(ptr::New<type::ClassInstance>(class_type));
                return class_var;
            }

            template<typename ...JArgs>
            static Ptr<Variable> NewClassVariable(Ptr<ClassType> class_type, const String &init_descriptor, JArgs &&...java_args) {
                auto class_var = ptr::New<Variable>(ptr::New<type::ClassInstance>(class_type));
                
                auto class_obj = class_var->GetAs<type::ClassInstance>();
                class_obj->CallConstructor(class_var, init_descriptor, java_args...);

                return class_var;
            }

            static inline Ptr<Variable> NewArray(u32 length, VariableType type) {
                return ptr::New<Variable>(ptr::New<type::Array>(type, length));
            }

            static inline Ptr<Variable> NewArray(u32 length, Ptr<ClassType> type) {
                return ptr::New<Variable>(ptr::New<type::Array>(type, length));
            }

            template<typename ...JArgs>
            static inline Ptr<Variable> NewArray(VariableType type, JArgs &&...java_args) {
                auto arr_obj = ptr::New<type::Array>(type, sizeof...(JArgs));
                u32 idx = 0;
                (SetInArray(java_args, arr_obj, idx), ...);
                return ptr::New<Variable>(arr_obj);
            }

            template<typename ...JArgs>
            static inline Ptr<Variable> NewMultiArray(VariableType type, std::vector<u32> dimension_lengths) {
                auto arr_len = dimension_lengths.front();
                dimension_lengths.erase(dimension_lengths.begin());
                return ptr::New<Variable>(ptr::New<type::Array>(arr_len, dimension_lengths, type));
            }

            static inline Ptr<Variable> True() {
                return NewPrimitiveVariable<type::Integer>(1);
            }

            static inline Ptr<Variable> False() {
                return NewPrimitiveVariable<type::Integer>(0);
            }

            static inline Ptr<Variable> Null() {
                return GetNullVariableImpl();
            }

            // New java.lang.Class from reflection type
            static inline Ptr<Variable> NewClassTypeVariable(Ptr<ReflectionType> ref_type) {
                auto class_name = ref_type->GetTypeName();
                auto cached_v = GetCachedClassType(class_name);
                if(cached_v) {
                    return cached_v;
                }

                auto class_class_type = inner_impl::LocateClassTypeImpl(u"java/lang/Class");

                // No need to call ctor, we set classLoader to null manually to avoid having to execute code
                auto class_v = NewClassVariable(class_class_type);
                auto class_obj = class_v->GetAs<type::ClassInstance>();

                auto class_name_v = inner_impl::CreateNewString(class_name);

                class_obj->SetField(u"classLoader", u"Ljava/lang/ClassLoader;", Null());
                class_obj->SetField(u"name", u"Ljava/lang/String;", class_name_v);

                CacheClassType(class_v);
                return class_v;
            }

            // Extras

            static String FormatVariableType(Ptr<Variable> var) {
                if(!var) {
                    return u"<invalid>";
                }
                auto type = var->GetType();
                if(var->IsNull()) {
                    return u"<null>";
                }
                if(IsPrimitiveType(type)) {
                    return TypeTraits::GetNameForPrimitiveType(type);
                }
                else {
                    if(type == VariableType::ClassInstance) {
                        auto class_obj = var->GetAs<type::ClassInstance>();
                        return ClassUtils::MakeDotClassName(class_obj->GetClassType()->GetClassName());
                    }
                    else if(type == VariableType::Array) {
                        auto arr_obj = var->GetAs<type::Array>();
                        String base_s;
                        if(arr_obj->IsClassInstanceArray()) {
                            auto cls_type = arr_obj->GetClassType();
                            base_s += ClassUtils::MakeDotClassName(cls_type->GetClassName());
                        }
                        else {
                            base_s += TypeTraits::GetNameForPrimitiveType(arr_obj->GetVariableType());
                        }
                        auto len = arr_obj->GetLength();
                        auto base = base_s + u"[" + StrUtils::From(len) + u"]";
                        return base;
                    }
                }
                return u"<unknown - type: " + StrUtils::From(static_cast<u32>(type)) + u">";
            }

            static String FormatVariable(Ptr<Variable> var) {
                if(!var) {
                    return u"<invalid>";
                }
                if(var->IsNull()) {
                    return u"<null>";
                }
                auto type = var->GetType();
                if(IsPrimitiveType(type)) {
                    switch(type) {
                        case VariableType::Integer:
                        case VariableType::Character:
                        case VariableType::Boolean:
                        case VariableType::Byte:
                        case VariableType::Short: {
                            auto val = var->GetValue<type::Integer>();
                            return StrUtils::From(val);
                        }
                        case VariableType::Float: {
                            auto val = var->GetValue<type::Float>();
                            return StrUtils::From(val);
                        }
                        case VariableType::Double: {
                            auto val = var->GetValue<type::Double>();
                            return StrUtils::From(val);
                        }
                        case VariableType::Long: {
                            auto val = var->GetValue<type::Long>();
                            return StrUtils::From(val);
                        }
                        default:
                            return u"<wtf>";
                    }
                }
                else {
                    if(type == VariableType::ClassInstance) {
                        auto class_obj = var->GetAs<type::ClassInstance>();
                        auto ret = class_obj->CallInstanceMethod(u"toString", u"()Ljava/lang/String;", var);
                        return inner_impl::GetStringValue(ret.ret_var);
                    }
                    if(type == VariableType::Array) {
                        return u"<array>";
                    }
                }
                return u"<unknown - type: " + StrUtils::From(static_cast<u32>(type)) + u">";
            }

            static String GetBaseClassName(const String &class_name) {
                auto name_copy = class_name;
                while(name_copy.front() == u'[') {
                    name_copy.erase(0, 1);
                }
                while(name_copy.front() == u'L') {
                    name_copy.erase(0, 1);
                }
                if(name_copy.back() == u';') {
                    name_copy.pop_back();
                }
                return ClassUtils::MakeSlashClassName(name_copy);
            }

    };

    namespace inner_impl {

        inline Ptr<Variable> NewDefaultVariableImpl(VariableType type) {
            return TypeUtils::NewDefaultVariable(type);
        }

        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type) {
            return TypeUtils::NewClassVariable(class_type);
        }

        template<typename ...JArgs>
        inline Ptr<Variable> NewClassVariableImpl(Ptr<ClassType> class_type, const String &init_descriptor, JArgs &&...java_args) {
            return TypeUtils::NewClassVariable(class_type, init_descriptor, java_args...);
        }
        
    }

}