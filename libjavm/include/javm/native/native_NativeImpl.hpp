
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native {

    using namespace vm;

    namespace impl {

        namespace inner_impl {

            static std::map<String, type::Integer> g_signal_table = {
                // TODO
            };

            static inline std::map<String, type::Integer> &GetSignalTable() {
                return g_signal_table;
            }

        }

        inline std::pair<type::Integer, bool> DecodeUnsafeOffset(type::Long raw_off) {
            union {
                struct {
                    type::Integer is_static;
                    type::Integer offset;
                };
                type::Long raw_offset;
            } offset{};
            offset.raw_offset = raw_off;
            return std::make_pair(offset.offset, (bool)offset.is_static);
        }

        inline type::Long EncodeUnsafeOffset(type::Integer off, bool is_static) {
            union {
                struct {
                    type::Integer is_static;
                    type::Integer offset;
                };
                type::Long raw_offset;
            } offset{};
            offset.offset = off;
            offset.is_static = (type::Integer)is_static;
            return offset.raw_offset;
        }

        inline u16 GetClassModifiersImpl(Ptr<ClassType> class_type) {
            if(class_type) {
                return class_type->GetAccessFlags();
            }
            return static_cast<u16>(AccessFlagUtils::Make(AccessFlags::Public, AccessFlags::Final, AccessFlags::Abstract));
        }

        inline Ptr<ReflectionType> GetReflectionTypeFromClassVariable(Ptr<Variable> var) {
            if(var->CanGetAs<VariableType::ClassInstance>()) {
                auto var_obj = var->GetAs<type::ClassInstance>();
                auto name_v = var_obj->GetField(u"name", u"Ljava/lang/String;");
                auto name = StringUtils::GetValue(name_v);
                return ReflectionUtils::FindTypeByName(name);
            }
            return nullptr;
        }

        ExecutionResult GetClassModifiers(Ptr<Variable> class_var) {
            auto ref_type = GetReflectionTypeFromClassVariable(class_var);
            if(ref_type) {
                JAVM_LOG("[native-GetClassModifiers] reflection type name: '%s'", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                if(ref_type->IsClassInstance()) {
                    auto class_type = ref_type->GetClassType();
                    auto flags_v = TypeUtils::NewPrimitiveVariable<type::Integer>(GetClassModifiersImpl(class_type));
                    return ExecutionResult::ReturnVariable(flags_v);
                }
            }
            u16 default_flags = static_cast<u16>(AccessFlagUtils::Make(AccessFlags::Abstract, AccessFlags::Final, AccessFlags::Public));
            auto flags_v = TypeUtils::NewPrimitiveVariable<type::Integer>(default_flags);
            return ExecutionResult::ReturnVariable(flags_v);
        }

        namespace java::lang::Object {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Object.registerNatives] called...");
                return ExecutionResult::Void();
            }

            ExecutionResult getClass(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    JAVM_LOG("[java.lang.Object.getClass] called - class type name: '%s'", StrUtils::ToUtf8(this_obj->GetClassType()->GetClassName()).c_str());
                    auto ref_type = ReflectionUtils::FindTypeByName(this_obj->GetClassType()->GetClassName());
                    JAVM_LOG("[java.lang.Object.getClass] reflection type name: '%s'", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(ref_type));
                }
                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            ExecutionResult hashCode(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                    auto this_obj = this_var->GetAs<type::ClassInstance>();
                    uintptr_t this_obj_ptr = (uintptr_t)this_obj.get();
                    auto hash_code = static_cast<type::Integer>(this_obj_ptr);
                    JAVM_LOG("[java.lang.Object.hashCode] called - hash code: %d", hash_code);
                    return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(hash_code));
                }
                else {
                    JAVM_LOG("[java.lang.Object.getClass] Variable isn't class instance - W T F?");
                }
                return ExecutionResult::Void();
            }

            ExecutionResult notify(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Object.notify] called...");
                auto this_obj = this_var->GetAs<type::ClassInstance>();
                auto monitor = this_obj->GetMonitor();
                monitor->Notify();
                return ExecutionResult::Void();
            }

            ExecutionResult notifyAll(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Object.notifyAll] called...");
                auto this_obj = this_var->GetAs<type::ClassInstance>();
                auto monitor = this_obj->GetMonitor();
                monitor->NotifyAll();
                return ExecutionResult::Void();
            }

            ExecutionResult wait(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto timeout_v = param_vars[0];
                auto timeout = timeout_v->GetValue<type::Long>();
                JAVM_LOG("[java.lang.Object.wait] called...");
                auto this_obj = this_var->GetAs<type::ClassInstance>();
                auto monitor = this_obj->GetMonitor();
                monitor->WaitFor(timeout);
                return ExecutionResult::Void();
            }

        }

        namespace java::lang::System {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.System.registerNatives] called...");
                return ExecutionResult::Void();
            }

            ExecutionResult initProperties(std::vector<Ptr<Variable>> param_vars) {
                auto props_v = param_vars[0];
                auto props_obj = props_v->GetAs<type::ClassInstance>();

                for(auto &[key, value] : vm::GetSystemPropertyTable()) {
                    JAVM_LOG("[java.lang.System.initProperties] setting property '%s' of value '%s'", StrUtils::ToUtf8(key).c_str(), StrUtils::ToUtf8(value).c_str());
                    auto key_str = StringUtils::CreateNew(key);
                    auto val_str = StringUtils::CreateNew(value);
                    props_obj->CallInstanceMethod(u"setProperty", u"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;", props_v, key_str, val_str);
                }

                return ExecutionResult::ReturnVariable(props_v);
            }

            ExecutionResult arraycopy(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.System.arraycopy] called");
                
                if(param_vars.size() == 5) {
                    auto src_v = param_vars[0];
                    auto srcpos_v = param_vars[1];
                    auto dst_v = param_vars[2];
                    auto dstpos_v = param_vars[3];
                    auto len_v = param_vars[4];
                    if(src_v->CanGetAs<VariableType::Array>()) {
                        auto src = src_v->GetAs<type::Array>();
                        if(srcpos_v->CanGetAs<VariableType::Integer>()) {
                            auto srcpos = srcpos_v->GetValue<type::Integer>();
                            if(dst_v->CanGetAs<VariableType::Array>()) {
                                auto dst = dst_v->GetAs<type::Array>();
                                if(dstpos_v->CanGetAs<VariableType::Integer>()) {
                                    auto dstpos = dstpos_v->GetValue<type::Integer>();
                                    if(len_v->CanGetAs<VariableType::Integer>()) {
                                        auto len = len_v->GetValue<type::Integer>();
                                        // Create a temporary array, push values there, then move them to the dst array
                                        std::vector<Ptr<Variable>> tmp_values;
                                        tmp_values.reserve(len);
                                        for(int i = srcpos; i < (srcpos + len); i++) {
                                            tmp_values.push_back(src->GetAt(i));
                                        }
                                        for(int i = 0; i < len; i++) {
                                            dst->SetAt(i + dstpos, tmp_values[i]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                return ExecutionResult::Void();
            }

            ExecutionResult setIn0(std::vector<Ptr<Variable>> param_vars) {
                auto stream_v = param_vars[0];
                JAVM_LOG("[java.lang.System.setIn0] called - in stream: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(stream_v)).c_str());
                auto system_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/lang/System");
                system_class_type->SetStaticField(u"in", u"Ljava/io/InputStream;", stream_v);
                return ExecutionResult::Void();
            }

            ExecutionResult setOut0(std::vector<Ptr<Variable>> param_vars) {
                auto stream_v = param_vars[0];
                JAVM_LOG("[java.lang.System.setOut0] called - out stream: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(stream_v)).c_str());
                auto system_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/lang/System");
                system_class_type->SetStaticField(u"out", u"Ljava/io/PrintStream;", stream_v);
                return ExecutionResult::Void();
            }

            ExecutionResult setErr0(std::vector<Ptr<Variable>> param_vars) {
                auto stream_v = param_vars[0];
                JAVM_LOG("[java.lang.System.setErr0] called - err stream: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(stream_v)).c_str());
                auto system_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/lang/System");
                system_class_type->SetStaticField(u"err", u"Ljava/io/PrintStream;", stream_v);
                return ExecutionResult::Void();
            }

            ExecutionResult mapLibraryName(std::vector<Ptr<Variable>> param_vars) {
                auto lib_v = param_vars[0];
                auto lib = StringUtils::GetValue(lib_v);
                JAVM_LOG("[java.lang.System.mapLibraryName] called - library name: '%s'...", StrUtils::ToUtf8(lib).c_str());
                return ExecutionResult::ReturnVariable(lib_v);
            }

            ExecutionResult loadLibrary(std::vector<Ptr<Variable>> param_vars) {
                auto lib_v = param_vars[0];
                auto lib = StringUtils::GetValue(lib_v);
                JAVM_LOG("[java.lang.System.loadLibrary] called - library name: '%s'...", StrUtils::ToUtf8(lib).c_str());
                return ExecutionResult::Void();
            }

            ExecutionResult currentTimeMillis(std::vector<Ptr<Variable>> param_vars) {
                timeval time = {};
                gettimeofday(&time, nullptr);
                auto time_ms = time.tv_sec * 1000 + time.tv_usec / 1000;
                JAVM_LOG("[java.lang.System.currentTimeMillis] called - time ms: %ld", time_ms);
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(time_ms));
            }

        }

        namespace java::lang::Class {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.registerNatives] called...");
                return ExecutionResult::Void();
            }

            ExecutionResult getPrimitiveClass(std::vector<Ptr<Variable>> param_vars) {
                auto type_name_v = param_vars[0];
                auto type_name = StringUtils::GetValue(type_name_v);
                JAVM_LOG("[java.lang.Class.getPrimitiveClass] called - primitive type name: '%s'...", StrUtils::ToUtf8(type_name).c_str());
                auto ref_type = ReflectionUtils::FindTypeByName(type_name);
                if(ref_type) {
                    return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(ref_type));
                }
                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            ExecutionResult desiredAssertionStatus0(std::vector<Ptr<Variable>> param_vars) {
                auto ref_type = GetReflectionTypeFromClassVariable(param_vars[0]);
                if(ref_type) {
                    JAVM_LOG("[java.lang.Class.desiredAssertionStatus0] called - Reflection type name: '%s'...", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                }
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(0));
            }

            ExecutionResult forName0(std::vector<Ptr<Variable>> param_vars) {
                auto class_name_v = param_vars[0];
                auto class_name = StringUtils::GetValue(class_name_v);
                auto init_v = param_vars[1];
                auto init = init_v->GetAs<type::Boolean>();
                auto loader_v = param_vars[2];
                auto loader = loader_v->GetAs<type::ClassInstance>();

                JAVM_LOG("[java.lang.Class.forName0] called - Class name: '%s'...", StrUtils::ToUtf8(class_name).c_str());

                auto ref_type = ReflectionUtils::FindTypeByName(class_name);
                if(ref_type) {
                    JAVM_LOG("[java.lang.Class.desiredAssertionStatus0] called - Processed reflection type: '%s'...", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    // Call the static initializer, just in case
                    if(ref_type->IsClassInstance()) {
                        auto class_type = ref_type->GetClassType();
                        auto ret = class_type->EnsureStaticInitializerCalled();
                        if(ret.IsInvalidOrThrown()) {
                            return ret;
                        }
                    }
                    return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(ref_type));
                }

                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            ExecutionResult getDeclaredFields0(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.getDeclaredFields0] called...");
                auto public_only_v = param_vars[0];

                auto ref_type = GetReflectionTypeFromClassVariable(this_var);
                if(ref_type) {
                    JAVM_LOG("[java.lang.Class.getDeclaredFields0] reflection type name: '%s'...", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    if(ref_type->IsClassInstance()) {
                        auto class_type = ref_type->GetClassType();
                        auto field_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/lang/reflect/Field");
                        if(field_class_type) {
                            if(class_type) {
                                auto &fields = class_type->GetRawFields();
                                const size_t field_count = fields.size();
                                auto field_array = TypeUtils::NewArray(field_count, field_class_type);
                                auto field_array_obj = field_array->GetAs<type::Array>();
                                for(u32 i = 0; i < field_count; i++) {
                                    auto &field = fields[i];
                                    auto field_v = TypeUtils::NewClassVariable(field_class_type);
                                    auto field_obj = field_v->GetAs<type::ClassInstance>();

                                    auto declaring_class_obj = this_var;
                                    JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field[%d] -> %s", i, StrUtils::ToUtf8(field.GetName()).c_str());

                                    field_obj->SetField(u"clazz", u"Ljava/lang/Class;", declaring_class_obj);

                                    auto field_ref_type = ReflectionUtils::FindTypeByName(field.GetDescriptor());
                                    if(field_ref_type) {
                                        JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field reflection type: '%s'...", StrUtils::ToUtf8(field_ref_type->GetTypeName()).c_str());
                                        auto class_v = TypeUtils::NewClassTypeVariable(field_ref_type);
                                        field_obj->SetField(u"type", u"Ljava/lang/Class;", class_v);
                                    }

                                    auto offset = class_type->GetRawFieldUnsafeOffset(field.GetName(), field.GetDescriptor());
                                    field_obj->SetField(u"slot", u"I", TypeUtils::NewPrimitiveVariable<type::Integer>(offset));

                                    auto name = StringUtils::CreateNew(field.GetName());
                                    field_obj->SetField(u"name", u"Ljava/lang/String;", name);

                                    auto modifiers = (type::Integer)field.GetAccessFlags();
                                    field_obj->SetField(u"modifiers", u"I", TypeUtils::NewPrimitiveVariable<type::Integer>(modifiers));

                                    auto signature = StringUtils::CreateNew(field.GetDescriptor());
                                    field_obj->SetField(u"signature", u"Ljava/lang/String;", signature);

                                    field_obj->SetField(u"override", u"Z", TypeUtils::False());

                                    field_array_obj->SetAt(i, field_v);
                                }
                                JAVM_LOG("[java.lang.Class.getDeclaredFields0] Field count: %d", field_array_obj->GetLength());
                                return ExecutionResult::ReturnVariable(field_array);
                            }
                            else {
                                // No type / no fields (primitive type), let's return an empty array
                                auto field_array = TypeUtils::NewArray(0, field_class_type);
                                return ExecutionResult::ReturnVariable(field_array);
                            }
                        }
                    }
                }
                
                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            ExecutionResult isInterface(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.isInterface] called...");

                auto ref_type = GetReflectionTypeFromClassVariable(this_var);
                if(ref_type) {
                    JAVM_LOG("[java.lang.Class.isInterface] reflection type name: '%s'...", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    if(ref_type->IsClassInstance()) {
                        auto class_type = ref_type->GetClassType();
                        if(class_type->HasFlag<AccessFlags::Interface>()) {
                            return ExecutionResult::ReturnVariable(TypeUtils::True());    
                        }
                    }
                }
                
                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            ExecutionResult isPrimitive(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.isPrimitive] called...");

                auto ref_type = GetReflectionTypeFromClassVariable(this_var);
                if(ref_type) {
                    JAVM_LOG("[java.lang.Class.isPrimitive] reflection type name: '%s'...", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    if(ref_type->IsPrimitive()) {
                        return ExecutionResult::ReturnVariable(TypeUtils::True());    
                    }
                }
                
                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            ExecutionResult isAssignableFrom(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.isAssignableFrom] called...");

                auto ref_type_1 = GetReflectionTypeFromClassVariable(this_var);
                if(ref_type_1) {
                    JAVM_LOG("[java.lang.Class.isAssignableFrom] reflection 1 type name: '%s'...", StrUtils::ToUtf8(ref_type_1->GetTypeName()).c_str());
                    auto ref_type_2 = GetReflectionTypeFromClassVariable(param_vars[0]);
                    if(ref_type_2) {
                        JAVM_LOG("[java.lang.Class.isAssignableFrom] reflection 2 type name: '%s'...", StrUtils::ToUtf8(ref_type_2->GetTypeName()).c_str());
                        if(ref_type_1->IsClassInstance() && ref_type_2->IsClassInstance()) {
                            auto class_1 = ref_type_1->GetClassType();
                            auto class_2 = ref_type_2->GetClassType();
                            if(class_1->CanCastTo(class_2->GetClassName())) {
                                return ExecutionResult::ReturnVariable(TypeUtils::True());
                            }
                        }
                    }
                }
                
                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            ExecutionResult getModifiers(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Class.getModifiers] called");
                return GetClassModifiers(this_var);
            }

        }

        namespace java::lang::ClassLoader {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.ClassLoader.registerNatives] called...");
                return ExecutionResult::Void();
            }

        }

        namespace java::security::AccessController {

            ExecutionResult doPrivileged(std::vector<Ptr<Variable>> param_vars) {
                auto action_v = param_vars[0];
                JAVM_LOG("[java.security.AccessController.doPrivileged] called - action type: '%s'", StrUtils::ToUtf8(TypeUtils::FormatVariableType(action_v)).c_str());
                auto action_obj = action_v->GetAs<type::ClassInstance>();
                auto ret = action_obj->CallInstanceMethod(u"run", u"()Ljava/lang/Object;", action_v);
                JAVM_LOG("Return type of privileged action: %d", static_cast<u32>(ret.status));
                return ret;
            }

            ExecutionResult getStackAccessControlContext(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.security.AccessController.getStackAccessControlContext] called");
                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

        }

        namespace java::lang::Float {

            ExecutionResult floatToRawIntBits(std::vector<Ptr<Variable>> param_vars) {
                auto f_v = param_vars[0];
                auto f_flt = f_v->GetValue<type::Float>();
                JAVM_LOG("[java.lang.Float.floatToRawIntBits] called - float: %f", f_flt);
                // Do the conversion :P
                union {
                    int i;
                    float flt;
                } float_conv{};
                float_conv.flt = f_flt;

                auto res_i = float_conv.i;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(res_i));
            }

        }

        namespace java::lang::Double {

            ExecutionResult doubleToRawLongBits(std::vector<Ptr<Variable>> param_vars) {
                auto d_v = param_vars[0];
                auto d_dbl = d_v->GetValue<type::Double>();
                JAVM_LOG("[java.lang.Double.doubleToRawLongBits] called - double: %f", d_dbl);
                // Do the conversion :P
                union {
                    long l;
                    double dbl;
                } double_conv{};
                double_conv.dbl = d_dbl;

                auto res_l = double_conv.l;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(res_l));
            }

            ExecutionResult longBitsToDouble(std::vector<Ptr<Variable>> param_vars) {
                auto l_v = param_vars[0];
                auto l_long = l_v->GetValue<type::Long>();
                JAVM_LOG("[java.lang.Double.longBitsToDouble] called - long: %ld", l_long);
                // Do the conversion :P
                union {
                    long l;
                    double dbl;
                } double_conv{};
                double_conv.l = l_long;

                auto res_d = double_conv.dbl;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Double>(res_d));
            }

        }

        namespace sun::misc::VM {

            ExecutionResult initialize(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.VM.initialize] called");
                return ExecutionResult::Void();
            }

        }
        
        namespace sun::reflect::Reflection {

            ExecutionResult getCallerClass(std::vector<Ptr<Variable>> param_vars) {
                auto thr = ThreadUtils::GetCurrentThread();
                JAVM_LOG("[sun.reflect.Reflection.getCallerClass] called...");

                for(auto call_info: thr->GetInvertedCallStack()) {
                    // Find the currently invoked method, check if it is '@CallerSensitive'
                    bool found = false;
                    for(auto &raw_inv: call_info.caller_type->GetRawInvokables()) {
                        if(raw_inv.GetName() == call_info.invokable_name) {
                            if(raw_inv.GetDescriptor() == call_info.invokable_desc) {
                                found = !raw_inv.HasAnnotation(u"Lsun/reflect/CallerSensitive;");
                                break;
                            }
                        }
                    }
                    if(found) {
                        // This one is a valid one
                        JAVM_LOG("[sun.reflect.Reflection.getCallerClass] called - caller class type: '%s'...", StrUtils::ToUtf8(call_info.caller_type->GetClassName()).c_str());

                        auto dummy_ref_type = ReflectionUtils::FindTypeByName(call_info.caller_type->GetClassName());
                        return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(dummy_ref_type));
                    }
                }

                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            ExecutionResult getClassAccessFlags(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.reflect.Reflection.getClassAccessFlags] called");
                return GetClassModifiers(param_vars[0]);
            }

        }

        namespace sun::misc::Unsafe {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.registerNatives] called");
                return ExecutionResult::Void();
            }

            ExecutionResult arrayBaseOffset(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.arrayBaseOffset] called");
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(0));
            }

            ExecutionResult arrayIndexScale(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.arrayIndexScale] called");
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
            }

            ExecutionResult addressSize(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.addressSize] called");
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
            }

            ExecutionResult objectFieldOffset(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] called");
                auto field_v = param_vars[0];
                auto field_obj = field_v->GetAs<type::ClassInstance>();
                auto field_name_v = field_obj->GetField(u"name", u"Ljava/lang/String;");
                auto field_name = StringUtils::GetValue(field_name_v);
                auto field_desc_v = field_obj->GetField(u"signature", u"Ljava/lang/String;");
                auto field_desc = StringUtils::GetValue(field_desc_v);
                auto class_type_v = field_obj->GetField(u"clazz", u"Ljava/lang/Class;");
                auto ref_type = GetReflectionTypeFromClassVariable(class_type_v);
                if(ref_type) {
                    JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] reflection type name: '%s'", StrUtils::ToUtf8(ref_type->GetTypeName()).c_str());
                    if(ref_type->IsClassInstance()) {
                        auto class_type = ref_type->GetClassType();
                        auto offset = class_type->GetRawFieldUnsafeOffset(field_name, field_desc);
                        auto is_static = class_type->IsRawFieldStatic(field_name, field_desc);
                        auto raw_offset = EncodeUnsafeOffset(offset, is_static);
                        JAVM_LOG("[sun.misc.Unsafe.objectFieldOffset] field: '%s' - '%s', offset: %d", StrUtils::ToUtf8(field_name).c_str(), StrUtils::ToUtf8(field_desc).c_str(), offset);
                        ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(raw_offset));
                    }
                }
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(0));
            }

            ExecutionResult getIntVolatile(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto obj_v = param_vars[0];
                auto obj_obj = obj_v->GetAs<type::ClassInstance>();
                auto obj_type = obj_obj->GetClassType();
                auto raw_off_v = param_vars[1];
                auto raw_off = raw_off_v->GetValue<type::Long>();
                auto [off, is_static] = DecodeUnsafeOffset(raw_off);
                JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] called - offset: %d, static: %s", off, is_static ? "true" : "false");

                if(is_static) {
                    auto var = obj_type->GetStaticFieldByUnsafeOffset(off);
                    JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] returning '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str());
                    return ExecutionResult::ReturnVariable(var);
                }
                else {
                    auto var = obj_obj->GetFieldByUnsafeOffset(off);
                    JAVM_LOG("[sun.misc.Unsafe.getIntVolatile] returning '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str());
                    return ExecutionResult::ReturnVariable(var);
                }

                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(sizeof(intptr_t)));
            }

            template<typename T>
            type::Boolean DoCompareAndSwap(Ptr<T> ptr, T old_v, T new_v) {
                if(PtrUtils::GetValue(ptr) == old_v) {
                    PtrUtils::SetValue(ptr, new_v);
                    return true;
                }
                else {
                    PtrUtils::SetValue(ptr, old_v);
                    return false;
                }
            }

            ExecutionResult compareAndSwapInt(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Unsafe.compareAndSwapInt] called");
                auto var_v = param_vars[0];
                auto var_obj = var_v->GetAs<type::ClassInstance>();
                auto raw_off_v = param_vars[1];
                auto raw_off = raw_off_v->GetValue<type::Long>();
                auto expected_v = param_vars[2];
                auto expected = expected_v->GetValue<type::Integer>();
                auto newv_v = param_vars[3];
                auto newv = newv_v->GetValue<type::Integer>();
                auto [off, is_static] = DecodeUnsafeOffset(raw_off);
                Ptr<Variable> field_v;
                if(is_static) {
                    field_v = var_obj->GetClassType()->GetStaticFieldByUnsafeOffset(off);
                }
                else {
                    field_v = var_obj->GetFieldByUnsafeOffset(off);
                }
                if(field_v) {
                    auto field_ref = field_v->GetAs<type::Integer>();
                    JAVM_LOG("[sun.misc.Unsafe.compareAndSwapInt] Offset: %d, Static: %s, Value: %d, Expected: %d, New: %d", off, is_static ? "true" : "false", PtrUtils::GetValue(field_ref), expected, newv);
                    auto ret = DoCompareAndSwap(field_ref, expected, newv);
                    field_v->SetAs<type::Integer>(field_ref);
                    if(ret) {
                        return ExecutionResult::ReturnVariable(TypeUtils::True());
                    }
                }

                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            ExecutionResult allocateMemory(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto size_v = param_vars[0];
                auto size = size_v->GetValue<type::Long>();
                auto ptr = new u8[size]();
                JAVM_LOG("[sun.misc.Unsafe.allocateMemory] called - Size: %ld, Ptr: %p", size, ptr);
                auto ptr_val = (type::Long)ptr;
                auto ptr_v = TypeUtils::NewPrimitiveVariable<type::Long>(ptr_val);
                return ExecutionResult::ReturnVariable(ptr_v);
            }

            ExecutionResult putLong(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto addr_v = param_vars[0];
                auto addr = addr_v->GetValue<type::Long>();
                auto val_v = param_vars[1];
                auto val = val_v->GetValue<type::Long>();
                auto ptr = (type::Long*)addr;
                JAVM_LOG("[sun.misc.Unsafe.putLong] called - Addr: %p, Value: %ld", ptr, val);
                *ptr = val;
                return ExecutionResult::Void();
            }

            ExecutionResult getByte(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto addr_v = param_vars[0];
                auto addr = addr_v->GetValue<type::Long>();
                auto ptr = (u8*)addr;
                JAVM_LOG("[sun.misc.Unsafe.getByte] called - Addr: %p", ptr);
                auto byte = *ptr;
                auto byte_v = TypeUtils::NewPrimitiveVariable<type::Byte>((type::Byte)byte);
                return ExecutionResult::ReturnVariable(byte_v);
            }

            ExecutionResult freeMemory(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto addr_v = param_vars[0];
                auto addr = addr_v->GetValue<type::Long>();
                auto ptr = (u8*)addr;
                JAVM_LOG("[sun.misc.Unsafe.freeMemory] called - Addr: %p", ptr);
                delete[] ptr;
                return ExecutionResult::Void();
            }

        }

        namespace java::lang::Throwable {

            ExecutionResult fillInStackTrace(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Throwable.fillInStackTrace] called");
                return ExecutionResult::ReturnVariable(this_var);
            }

        }

        namespace java::lang::String {

            ExecutionResult intern(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.String.intern] called - string: '%s'", StrUtils::ToUtf8(StringUtils::GetValue(this_var)).c_str());
                return ExecutionResult::ReturnVariable(StringUtils::CheckIntern(this_var));
            }

        }

        namespace java::io::FileDescriptor {

            ExecutionResult initIDs(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.io.FileDescriptor.initIDs] called");
                return ExecutionResult::Void();
            }

            ExecutionResult set(std::vector<Ptr<Variable>> param_vars) {
                auto fd_v = param_vars[0];
                auto fd_i = fd_v->GetValue<type::Integer>();
                JAVM_LOG("[java.io.FileDescriptor.set] called - fd: %d", fd_i);
                auto fd_l_v = TypeUtils::NewPrimitiveVariable<type::Long>((type::Long)fd_i);
                return ExecutionResult::ReturnVariable(fd_l_v);
            }

        }

        namespace java::lang::Thread {

            ExecutionResult registerNatives(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.lang.Thread.registerNatives] called");
                return ExecutionResult::Void();
            }
            
            ExecutionResult currentThread(std::vector<Ptr<Variable>> param_vars) {
                auto thread_v = ThreadUtils::GetCurrentThreadInstance();
                JAVM_LOG("[java.lang.Thread.currentThread] called");

                return ExecutionResult::ReturnVariable(thread_v);
            }

            ExecutionResult setPriority0(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto prio_v = param_vars[0];
                auto prio = prio_v->GetValue<type::Integer>();
                JAVM_LOG("[java.lang.Thread.setPriority0] called - priority: %d", prio);

                auto thread_obj = this_var->GetAs<type::ClassInstance>();
                auto eetop_v = thread_obj->GetField(u"eetop", u"J");
                auto eetop = eetop_v->GetValue<type::Long>();

                native::SetThreadPriority(eetop, prio);

                return ExecutionResult::Void();
            }

            ExecutionResult isAlive(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto thread_obj = this_var->GetAs<type::ClassInstance>();
                auto name_res = thread_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
                if(name_res.IsInvalidOrThrown()) {
                    return name_res;
                }
                auto name_v = name_res.ret_var;
                auto name = StringUtils::GetValue(name_v);
                auto eetop_v = thread_obj->GetField(u"eetop", u"J");
                auto eetop = eetop_v->GetValue<type::Long>();

                auto thr = ThreadUtils::GetThreadByHandle(eetop);
                if(thr) {
                    JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' Thread name: '%s', thread handle: %ld", StrUtils::ToUtf8(name).c_str(), StrUtils::ToUtf8(thr->GetThreadName()).c_str(), eetop);
                    if(thr->GetThreadObject()->IsAlive()) {
                        return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Boolean>(1));
                    }
                }
                else {
                    JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' thread handle: %ld - not found (must be finished)...", StrUtils::ToUtf8(name).c_str(), eetop);
                }

                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Boolean>(0));
            }

            ExecutionResult start0(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto thr_obj = this_var->GetAs<type::ClassInstance>();
                auto name_res = thr_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
                if(name_res.IsInvalidOrThrown()) {
                    return name_res;
                }
                auto name_v = name_res.ret_var;
                auto name = StringUtils::GetValue(name_v);

                JAVM_LOG("[java.lang.Thread.start0] called - thread name: '%s'", StrUtils::ToUtf8(name).c_str());

                ThreadUtils::RegisterAndStartThread(this_var);

                return ExecutionResult::Void();
            }

        }

        namespace java::io::FileInputStream {

            ExecutionResult initIDs(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.io.FileInputStream.initIDs] called");
                return ExecutionResult::Void();
            }

        }

        namespace java::io::FileOutputStream {

            ExecutionResult initIDs(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.io.FileOutputStream.initIDs] called");
                return ExecutionResult::Void();
            }

            ExecutionResult writeBytes(Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
                auto byte_arr_v = param_vars[0];
                auto byte_arr = byte_arr_v->GetAs<type::Array>();
                auto off_v = param_vars[1];
                auto off = off_v->GetValue<type::Integer>();
                auto len_v = param_vars[2];
                auto len = len_v->GetValue<type::Integer>();
                auto append_v = param_vars[3];
                auto append = (bool)append_v->GetValue<type::Integer>();

                JAVM_LOG("[java.io.FileOutputStream.writeBytes] called - Array: bytes[%d], Offset: %d, Length: %d, Append: %s", byte_arr->GetLength(), off, len, append ? "true" : "false");

                auto this_obj = this_var->GetAs<type::ClassInstance>();
                auto fd_fd_v = this_obj->GetField(u"fd", u"Ljava/io/FileDescriptor;");
                auto fd_fd_obj = fd_fd_v->GetAs<type::ClassInstance>();
                auto fd_v = fd_fd_obj->GetField(u"fd", u"I");
                auto fd = fd_v->GetValue<type::Integer>();

                JAVM_LOG("[java.io.FileOutputStream.writeBytes] FD: %d", fd);

                const u32 proper_len = std::min((u32)len, (u32)byte_arr->GetLength());

                u8 *tmpbuf = new u8[proper_len]();

                for(u32 i = 0; i < proper_len; i++) {
                    auto byte_v = byte_arr->GetAt(i);
                    auto byte = (u8)byte_v->GetValue<type::Integer>();
                    tmpbuf[i] = byte;
                }

                int ret = write(fd, tmpbuf, proper_len);
                JAVM_LOG("[java.io.FileOutputStream.writeBytes] Ret: %d", ret);

                delete[] tmpbuf;

                return ExecutionResult::Void();
            }

        }

        namespace sun::nio::cs::StreamEncoder {

            ExecutionResult forOutputStreamWriter(std::vector<Ptr<Variable>> param_vars) {
                auto stream_v = param_vars[0];
                auto obj_v = param_vars[1];
                auto cs_name_v = param_vars[2];
                auto cs_name = StringUtils::GetValue(cs_name_v);
                JAVM_LOG("[sun.nio.cs.StreamEncoder.forOutputStreamWriter] called - Charset name: '%s'...", StrUtils::ToUtf8(cs_name).c_str());
                auto se_class_type = vm::inner_impl::LocateClassTypeImpl(u"sun/nio/cs/StreamEncoder");
                auto cs_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/nio/charset/Charset");
                auto global_cs_v = cs_class_type->GetStaticField(u"defaultCharset", u"Ljava/nio/charset/Charset;");
                auto se_v = TypeUtils::NewClassVariable(se_class_type, u"(Ljava/io/OutputStream;Ljava/lang/Object;Ljava/nio/charset/Charset;)V", stream_v, obj_v, global_cs_v);
                return ExecutionResult::ReturnVariable(se_v);
            }

        }

        namespace java::io::WinNTFileSystem {

            ExecutionResult initIDs(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.io.WinNTFileSystem.initIDs] called");
                return ExecutionResult::Void();
            }

        }

        namespace java::util::concurrent::atomic::AtomicLong {

            ExecutionResult VMSupportsCS8(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[java.util.concurrent.atomic.AtomicLong.VMSupportsCS8] called");
                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

        }

        namespace sun::misc::Signal {

            ExecutionResult findSignal(std::vector<Ptr<Variable>> param_vars) {
                auto sig_v = param_vars[0];
                auto sig = StringUtils::GetValue(sig_v);
                JAVM_LOG("[sun.misc.Signal.findSignal] called - signal: '%s'...", StrUtils::ToUtf8(sig).c_str());

                auto &sig_table = inner_impl::GetSignalTable();
                auto it = sig_table.find(sig);
                if(it != sig_table.end()) {
                    return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(it->second));
                }

                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            ExecutionResult handle0(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Signal.handle0] called...");
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(2));
            }

        }

        namespace sun::io::Win32ErrorMode {

            ExecutionResult setErrorMode(std::vector<Ptr<Variable>> param_vars) {
                JAVM_LOG("[sun.misc.Signal.handle0] setErrorMode...");
                return ExecutionResult::ReturnVariable(param_vars[0]);
            }

        }

    }

    static inline void RegisterNativeStandardImplementation() {
        if(inner_impl::IsNativeRegistered()) {
            return;
        }

        RegisterNativeClassMethod(u"java/lang/Object", u"registerNatives", u"()V", &impl::java::lang::Object::registerNatives);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"getClass", u"()Ljava/lang/Class;", &impl::java::lang::Object::getClass);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"hashCode", u"()I", &impl::java::lang::Object::hashCode);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"notify", u"()V", &impl::java::lang::Object::notify);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"notifyAll", u"()V", &impl::java::lang::Object::notifyAll);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"wait", u"(J)V", &impl::java::lang::Object::wait);
        RegisterNativeClassMethod(u"java/lang/System", u"registerNatives", u"()V", &impl::java::lang::System::registerNatives);
        RegisterNativeClassMethod(u"java/lang/System", u"initProperties", u"(Ljava/util/Properties;)Ljava/util/Properties;", &impl::java::lang::System::initProperties);
        RegisterNativeClassMethod(u"java/lang/System", u"arraycopy", u"(Ljava/lang/Object;ILjava/lang/Object;II)V", &impl::java::lang::System::arraycopy);
        RegisterNativeClassMethod(u"java/lang/System", u"setIn0", u"(Ljava/io/InputStream;)V", &impl::java::lang::System::setIn0);
        RegisterNativeClassMethod(u"java/lang/System", u"setOut0", u"(Ljava/io/PrintStream;)V", &impl::java::lang::System::setOut0);
        RegisterNativeClassMethod(u"java/lang/System", u"setErr0", u"(Ljava/io/PrintStream;)V", &impl::java::lang::System::setErr0);
        RegisterNativeClassMethod(u"java/lang/System", u"mapLibraryName", u"(Ljava/lang/String;)Ljava/lang/String;", &impl::java::lang::System::mapLibraryName);
        RegisterNativeClassMethod(u"java/lang/System", u"loadLibrary", u"(Ljava/lang/String;)V", &impl::java::lang::System::loadLibrary);
        RegisterNativeClassMethod(u"java/lang/System", u"currentTimeMillis", u"()J", &impl::java::lang::System::currentTimeMillis);
        RegisterNativeClassMethod(u"java/lang/Class", u"registerNatives", u"()V", &impl::java::lang::Class::registerNatives);
        RegisterNativeClassMethod(u"java/lang/Class", u"getPrimitiveClass", u"(Ljava/lang/String;)Ljava/lang/Class;", &impl::java::lang::Class::getPrimitiveClass);
        RegisterNativeClassMethod(u"java/lang/Class", u"desiredAssertionStatus0", u"(Ljava/lang/Class;)Z", &impl::java::lang::Class::desiredAssertionStatus0);
        RegisterNativeClassMethod(u"java/lang/Class", u"forName0", u"(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;", &impl::java::lang::Class::forName0);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"getDeclaredFields0", u"(Z)[Ljava/lang/reflect/Field;", &impl::java::lang::Class::getDeclaredFields0);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isInterface", u"()Z", &impl::java::lang::Class::isInterface);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isPrimitive", u"()Z", &impl::java::lang::Class::isPrimitive);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isAssignableFrom", u"(Ljava/lang/Class;)Z", &impl::java::lang::Class::isAssignableFrom);
        RegisterNativeClassMethod(u"java/lang/ClassLoader", u"registerNatives", u"()V", &impl::java::lang::ClassLoader::registerNatives);
        RegisterNativeClassMethod(u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;", &impl::java::security::AccessController::doPrivileged);
        RegisterNativeClassMethod(u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedAction;)Ljava/lang/Object;", &impl::java::security::AccessController::doPrivileged);
        RegisterNativeClassMethod(u"java/security/AccessController", u"getStackAccessControlContext", u"()Ljava/security/AccessControlContext;", &impl::java::security::AccessController::getStackAccessControlContext);
        RegisterNativeClassMethod(u"java/lang/Float", u"floatToRawIntBits", u"(F)I", &impl::java::lang::Float::floatToRawIntBits);
        RegisterNativeClassMethod(u"java/lang/Double", u"doubleToRawLongBits", u"(D)J", &impl::java::lang::Double::doubleToRawLongBits);
        RegisterNativeClassMethod(u"java/lang/Double", u"longBitsToDouble", u"(J)D", &impl::java::lang::Double::longBitsToDouble);
        RegisterNativeClassMethod(u"sun/misc/VM", u"initialize", u"()V", &impl::sun::misc::VM::initialize);
        RegisterNativeClassMethod(u"sun/reflect/Reflection", u"getCallerClass", u"()Ljava/lang/Class;", &impl::sun::reflect::Reflection::getCallerClass);
        RegisterNativeClassMethod(u"sun/reflect/Reflection", u"getClassAccessFlags", u"(Ljava/lang/Class;)I", &impl::sun::reflect::Reflection::getClassAccessFlags);
        RegisterNativeClassMethod(u"sun/misc/Unsafe", u"registerNatives", u"()V", &impl::sun::misc::Unsafe::registerNatives);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"arrayBaseOffset", u"(Ljava/lang/Class;)I", &impl::sun::misc::Unsafe::arrayBaseOffset);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"arrayIndexScale", u"(Ljava/lang/Class;)I", &impl::sun::misc::Unsafe::arrayIndexScale);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"addressSize", u"()I", &impl::sun::misc::Unsafe::addressSize);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"objectFieldOffset", u"(Ljava/lang/reflect/Field;)J", &impl::sun::misc::Unsafe::objectFieldOffset);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"getIntVolatile", u"(Ljava/lang/Object;J)I", &impl::sun::misc::Unsafe::getIntVolatile);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"compareAndSwapInt", u"(Ljava/lang/Object;JII)Z", &impl::sun::misc::Unsafe::compareAndSwapInt);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"allocateMemory", u"(J)J", &impl::sun::misc::Unsafe::allocateMemory);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"putLong", u"(JJ)V", &impl::sun::misc::Unsafe::putLong);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"getByte", u"(J)B", &impl::sun::misc::Unsafe::getByte);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"freeMemory", u"(J)V", &impl::sun::misc::Unsafe::freeMemory);
        RegisterNativeInstanceMethod(u"java/lang/Throwable", u"fillInStackTrace", u"(I)Ljava/lang/Throwable;", &impl::java::lang::Throwable::fillInStackTrace);
        RegisterNativeInstanceMethod(u"java/lang/String", u"intern", u"()Ljava/lang/String;", &impl::java::lang::String::intern);
        RegisterNativeClassMethod(u"java/io/FileDescriptor", u"initIDs", u"()V", &impl::java::io::FileDescriptor::initIDs);
        RegisterNativeClassMethod(u"java/io/FileDescriptor", u"set", u"(I)J", &impl::java::io::FileDescriptor::set);
        RegisterNativeClassMethod(u"java/lang/Thread", u"registerNatives", u"()V", &impl::java::lang::Thread::registerNatives);
        RegisterNativeClassMethod(u"java/lang/Thread", u"currentThread", u"()Ljava/lang/Thread;", &impl::java::lang::Thread::currentThread);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"setPriority0", u"(I)V", &impl::java::lang::Thread::setPriority0);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"isAlive", u"()Z", &impl::java::lang::Thread::isAlive);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"start0", u"()V", &impl::java::lang::Thread::start0);
        RegisterNativeClassMethod(u"java/io/FileInputStream", u"initIDs", u"()V", &impl::java::io::FileInputStream::initIDs);
        RegisterNativeClassMethod(u"java/io/FileOutputStream", u"initIDs", u"()V", &impl::java::io::FileOutputStream::initIDs);
        RegisterNativeInstanceMethod(u"java/io/FileOutputStream", u"writeBytes", u"([BIIZ)V", &impl::java::io::FileOutputStream::writeBytes);
        RegisterNativeClassMethod(u"sun/nio/cs/StreamEncoder", u"forOutputStreamWriter", u"(Ljava/io/OutputStream;Ljava/lang/Object;Ljava/lang/String;)Lsun/nio/cs/StreamEncoder;", &impl::sun::nio::cs::StreamEncoder::forOutputStreamWriter);
        RegisterNativeClassMethod(u"java/io/WinNTFileSystem", u"initIDs", u"()V", &impl::java::io::WinNTFileSystem::initIDs);
        RegisterNativeClassMethod(u"java/util/concurrent/atomic/AtomicLong", u"VMSupportsCS8", u"()Z", &impl::java::util::concurrent::atomic::AtomicLong::VMSupportsCS8);
        RegisterNativeClassMethod(u"sun/misc/Signal", u"findSignal", u"(Ljava/lang/String;)I", &impl::sun::misc::Signal::findSignal);
        RegisterNativeClassMethod(u"sun/misc/Signal", u"handle0", u"(IJ)J", &impl::sun::misc::Signal::handle0);
        RegisterNativeClassMethod(u"sun/io/Win32ErrorMode", u"setErrorMode", u"(J)J", &impl::sun::io::Win32ErrorMode::setErrorMode);

        inner_impl::NotifyNativeRegistered();
    }

}