
#pragma once
#include <javm/vm/vm_Instructions.hpp>
#include <javm/vm/vm_Execution.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <cmath>
#include <javm/rt/rt_Context.hpp>

namespace javm::vm {

    namespace inner_impl {

        Ptr<ClassType> LocateClassTypeImpl(const String &class_name) {
            return rt::LocateClassType(class_name);
        }

    }

    class ExecutionUtils {

        public:
            static u32 GetFunctionDescriptorParameterCount(const String &descriptor) {
                auto tmp = descriptor.substr(descriptor.find_first_of(u'(') + 1);
                tmp = tmp.substr(0, tmp.find_last_of(u')'));
                bool parsing_class = false;
                u32 count = 0;
                for(auto &ch: tmp) {
                    if(ch == u'[') {
                        // Array, so not a new parameter
                        continue;
                    }
                    if(parsing_class) {
                        if(ch == u';') {
                            parsing_class = false;
                        }
                        continue;
                    }
                    else {
                        if(ch == u'L') {
                            parsing_class = true;
                        }
                    }
                    count++;
                }
                return count;
            }

            static std::vector<Ptr<Variable>> LoadClassMethodParameters(ExecutionFrame &frame, const String &descriptor) {
                std::vector<Ptr<Variable>> params;
                const u32 param_count = GetFunctionDescriptorParameterCount(descriptor);
                for(u32 i = 0; i < param_count; i++) {
                    auto param_var = frame.PopStack();
                    params.push_back(param_var);
                }
                // Params are read in inverse order!
                std::reverse(params.begin(), params.end());
                return params;
            }

            static inline bool FunctionIsVoid(const String &descriptor) {
                return descriptor.back() == u'V';
            }

            static std::pair<Ptr<Variable>, std::vector<Ptr<Variable>>> LoadInstanceMethodParameters(ExecutionFrame &frame, const String &descriptor) {
                std::vector<Ptr<Variable>> params;
                const u32 param_count = GetFunctionDescriptorParameterCount(descriptor);
                for(u32 i = 0; i < param_count; i++) {
                    auto param_var = frame.PopStack();
                    params.push_back(param_var);
                }
                auto this_var = frame.PopStack();
                // Params are read in inverse order!
                std::reverse(params.begin(), params.end());
                return std::make_pair(this_var, params);
            }

    };

    namespace exec_impl {

        static ExecutionResult HandleInstructionImpl(ExecutionFrame &frame) {
            Instruction inst = static_cast<Instruction>(frame.ReadCode<u8>());
            JAVM_LOG("Got instruction: 0x%X", static_cast<u8>(inst));

            auto &pos = frame.GetCodePosition();

            #define _JAVM_LOAD_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    u8 index = idx; \
                    auto var = frame.GetLocalAt((u32)index); \
                    JAVM_LOG("[*load] Loaded: '%s' at locals[%d]", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str(), index); \
                    if(var->CanGetAs<VariableType::Integer>()) { \
                        JAVM_LOG("[*load] Integer value: %d", var->GetValue<type::Integer>()); \
                    } \
                    frame.PushStack(var); \
                    break; \
                }

            #define _JAVM_LOAD_INSTRUCTION_INDEX_READ(instr) _JAVM_LOAD_INSTRUCTION(instr, frame.ReadCode<u8>())
            
            #define _JAVM_STORE_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    u8 index = idx; \
                    auto var = frame.PopStack(); \
                    JAVM_LOG("[*store] Stored: '%s' at locals[%d]", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str(), index); \
                    frame.SetLocalAt((u32)index, var); \
                    break; \
                }
            
            #define _JAVM_STORE_INSTRUCTION_INDEX_READ(instr) _JAVM_STORE_INSTRUCTION(instr, frame.ReadCode<u8>())

            #define _JAVM_LDC_BASE(idx) { \
                auto &const_pool = frame.GetThisConstantPool(); \
                auto const_item = const_pool.GetItemAt(idx); \
                if(const_item) { \
                    JAVM_LOG("[ldc-base] Tag: %d", static_cast<u32>(const_item->GetTag())); \
                    switch(const_item->GetTag()) { \
                        case ConstantPoolTag::Integer: { \
                            type::Integer value = const_item->GetIntegerData().integer; \
                            JAVM_LOG("[ldc-base] Int value: '%d'", value); \
                            auto var = TypeUtils::NewPrimitiveVariable<type::Integer>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Float: { \
                            type::Float value = const_item->GetFloatData().flt; \
                            JAVM_LOG("[ldc-base] Float value: '%f'", value); \
                            auto var = TypeUtils::NewPrimitiveVariable<type::Float>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Long: { \
                            type::Long value = const_item->GetLongData().lng; \
                            JAVM_LOG("[ldc-base] Long value: '%ld'", value); \
                            auto var = TypeUtils::NewPrimitiveVariable<type::Long>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::Double: { \
                            type::Double value = const_item->GetDoubleData().dbl; \
                            JAVM_LOG("[ldc-base] Double value: '%f'", value); \
                            auto var = TypeUtils::NewPrimitiveVariable<type::Double>(value); \
                            frame.PushStack(var); \
                            break; \
                        } \
                        case ConstantPoolTag::String: { \
                            auto str = const_item->GetStringData().processed_string; \
                            JAVM_LOG("[ldc-base] String value: '%s'", StrUtils::ToUtf8(str).c_str()); \
                            auto str_var = StringUtils::CreateNew(str); \
                            frame.PushStack(str_var); \
                            break; \
                        } \
                        case ConstantPoolTag::Class: { \
                            auto type_name = const_item->GetClassData().processed_name; \
                            JAVM_LOG("[ldc-base] Type name: '%s'", StrUtils::ToUtf8(type_name).c_str()); \
                            auto ref_type = ReflectionUtils::FindTypeByName(type_name); \
                            if(ref_type) { \
                                auto class_v = TypeUtils::NewClassTypeVariable(ref_type); \
                                frame.PushStack(class_v); \
                            } \
                            else { \
                                ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid or unsupported constant pool item"); \
                            } \
                            break; \
                        } \
                        default: \
                            break; \
                    } \
                } \
                else { \
                    ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid or unsupported constant pool item"); \
                } \
            }

            #define _JAVM_LDC_INSTRUCTION(instr, idx) \
            case Instruction::instr: { \
                    auto index = idx; \
                    _JAVM_LDC_BASE(index) \
                    break; \
                }

            #define _JAVM_ALOAD_INSTRUCTION(instr) \
            case Instruction::instr: { \
                    auto index_var = frame.PopStack(); \
                    auto index_obj = index_var->GetAs<type::Integer>(); \
                    auto index = PtrUtils::GetValue(index_obj); \
                    if(index < 0) { \
                        return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Negative array index"); \
                    } \
                    else { \
                        auto array_var = frame.PopStack(); \
                        if(array_var->CanGetAs<VariableType::Array>()) { \
                            auto array_obj = array_var->GetAs<type::Array>(); \
                            auto inner_var = array_obj->GetAt(index); \
                            if(inner_var) { \
                                frame.PushStack(inner_var); \
                            } \
                            else { \
                                return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid array index"); \
                            } \
                        } \
                        else { \
                            return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid array item"); \
                        } \
                    } \
                    break; \
                }

            #define _JAVM_ASTORE_INSTRUCTION(instr) \
            case Instruction::instr: { \
                    auto value = frame.PopStack(); \
                    auto index_var = frame.PopStack(); \
                    auto index_obj = index_var->GetAs<type::Integer>(); \
                    auto index = PtrUtils::GetValue(index_obj); \
                    if(index < 0) { \
                        return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Negative array index"); \
                    } \
                    else { \
                        auto array_var = frame.PopStack(); \
                        if(array_var->CanGetAs<VariableType::Array>()) { \
                            auto array_obj = array_var->GetAs<type::Array>(); \
                            array_obj->SetAt(index, value); \
                        } \
                        else { \
                            return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid array index"); \
                        } \
                    } \
                    break; \
                }

            #define _JAVM_OPERATOR_INSTRUCTION(instr, typ, op) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_val = var2->GetValue<typ>(); \
                    auto var1 = frame.PopStack(); \
                    auto var1_val = var1->GetValue<typ>(); \
                    auto res_var = TypeUtils::NewPrimitiveVariable<typ>(var1_val op var2_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_DIV_INSTRUCTION(instr, typ) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_val = var2->GetValue<typ>(); \
                    if(var2_val == 0) { \
                        return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/ArithmeticException", u"Divide by zero"); \
                    } \
                    auto var1 = frame.PopStack(); \
                    auto var1_val = var1->GetValue<typ>(); \
                    auto res_var = TypeUtils::NewPrimitiveVariable<typ>(var1_val / var2_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_NEG_INSTRUCTION(instr, type) \
            case Instruction::instr: { \
                    auto var = frame.PopStack(); \
                    auto var_obj = var->GetAs<type>(); \
                    auto var_val = PtrUtils::GetValue(var_obj); \
                    auto res_var = TypeUtils::NewPrimitiveVariable<type>(-var_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_CONVERSION_INSTRUCTION(instr, t1, t2) \
            case Instruction::instr: { \
                    auto var = frame.PopStack(); \
                    auto var_obj = var->GetAs<t1>(); \
                    auto var_val = PtrUtils::GetValue(var_obj); \
                    auto res_var = TypeUtils::NewPrimitiveVariable<t2>((t2)var_val); \
                    frame.PushStack(res_var); \
                    break; \
                }

            #define _JAVM_CMP_INSTRUCTION(instr, typ) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_obj = var2->GetAs<typ>(); \
                    auto var2_val = PtrUtils::GetValue(var2_obj); \
                    auto var1 = frame.PopStack(); \
                    auto var1_obj = var1->GetAs<typ>(); \
                    auto var1_val = PtrUtils::GetValue(var1_obj); \
                    if(var2_val > var1_val) { \
                        JAVM_LOG("[cmp]  %ld > %ld", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(-1); \
                        frame.PushStack(res_var); \
                    } \
                    else if(var2_val < var1_val) { \
                        JAVM_LOG("[cmp]  %ld < %ld", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(1); \
                        frame.PushStack(res_var); \
                    } \
                    else { \
                        JAVM_LOG("[cmp]  %ld == %ld", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(0); \
                        frame.PushStack(res_var); \
                    } \
                    break; \
                }

            #define _JAVM_FDCMP_INSTRUCTION(instr, typ, nanv) \
            case Instruction::instr: { \
                    auto var2 = frame.PopStack(); \
                    auto var2_obj = var2->GetAs<typ>(); \
                    auto var2_val = PtrUtils::GetValue(var2_obj); \
                    auto var1 = frame.PopStack(); \
                    auto var1_obj = var1->GetAs<typ>(); \
                    auto var1_val = PtrUtils::GetValue(var1_obj); \
                    if(std::isnan(var1_val) || std::isnan(var2_val)) { \
                        JAVM_LOG("[fdcmp]  %f or %f is NaN!?", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(nanv); \
                        frame.PushStack(res_var); \
                    } \
                    if(var2_val > var1_val) { \
                        JAVM_LOG("[fdcmp]  %f > %f", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(-1); \
                        frame.PushStack(res_var); \
                    } \
                    else if(var2_val < var1_val) { \
                        JAVM_LOG("[fdcmp]  %f < %f", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(1); \
                        frame.PushStack(res_var); \
                    } \
                    else { \
                        JAVM_LOG("[fdcmp]  %f == %f", var1_val, var2_val); \
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(0); \
                        frame.PushStack(res_var); \
                    } \
                    break; \
                }

            switch(inst) {
                case Instruction::NOP: {
                    // Do nothing :P
                    break;
                }
                case Instruction::ACONST_NULL: {
                    auto null = TypeUtils::Null();
                    frame.PushStack(null);
                    break;
                }
                case Instruction::ICONST_M1: {
                    auto im1 = TypeUtils::NewPrimitiveVariable<type::Integer>(-1);
                    frame.PushStack(im1);
                    break;
                }
                case Instruction::ICONST_0: {
                    auto i0 = TypeUtils::NewPrimitiveVariable<type::Integer>(0);
                    frame.PushStack(i0);
                    break;
                }
                case Instruction::ICONST_1: {
                    auto i1 = TypeUtils::NewPrimitiveVariable<type::Integer>(1);
                    frame.PushStack(i1);
                    break;
                }
                case Instruction::ICONST_2: {
                    auto i2 = TypeUtils::NewPrimitiveVariable<type::Integer>(2);
                    frame.PushStack(i2);
                    break;
                }
                case Instruction::ICONST_3: {
                    auto i3 = TypeUtils::NewPrimitiveVariable<type::Integer>(3);
                    frame.PushStack(i3);
                    break;
                }
                case Instruction::ICONST_4: {
                    auto i4 = TypeUtils::NewPrimitiveVariable<type::Integer>(4);
                    frame.PushStack(i4);
                    break;
                }
                case Instruction::ICONST_5: {
                    auto i5 = TypeUtils::NewPrimitiveVariable<type::Integer>(5);
                    frame.PushStack(i5);
                    break;
                }
                case Instruction::LCONST_0: {
                    auto l0 = TypeUtils::NewPrimitiveVariable<type::Long>(0);
                    frame.PushStack(l0);
                    break;
                }
                case Instruction::LCONST_1: {
                    auto l1 = TypeUtils::NewPrimitiveVariable<type::Long>(1);
                    frame.PushStack(l1);
                    break;
                }
                case Instruction::FCONST_0: {
                    auto f0 = TypeUtils::NewPrimitiveVariable<type::Float>(0.0f);
                    frame.PushStack(f0);
                    break;
                }
                case Instruction::FCONST_1: {
                    auto f1 = TypeUtils::NewPrimitiveVariable<type::Float>(1.0f);
                    frame.PushStack(f1);
                    break;
                }
                case Instruction::FCONST_2: {
                    auto f2 = TypeUtils::NewPrimitiveVariable<type::Float>(2.0f);
                    frame.PushStack(f2);
                    break;
                }
                case Instruction::DCONST_0: {
                    auto d0 = TypeUtils::NewPrimitiveVariable<type::Double>(0.0f);
                    frame.PushStack(d0);
                    break;
                }
                case Instruction::DCONST_1: {
                    auto d1 = TypeUtils::NewPrimitiveVariable<type::Double>(1.0f);
                    frame.PushStack(d1);
                    break;
                }
                case Instruction::BIPUSH: {
                    auto byte = frame.ReadCode<u8>();
                    auto i = TypeUtils::NewPrimitiveVariable<type::Integer>((type::Integer)byte);
                    frame.PushStack(i);
                    break;
                }
                case Instruction::SIPUSH: {
                    auto shrt = BE(frame.ReadCode<i16>());
                    auto i = TypeUtils::NewPrimitiveVariable<type::Integer>((type::Integer)shrt);
                    frame.PushStack(i);
                    break;
                }
                _JAVM_LDC_INSTRUCTION(LDC, frame.ReadCode<u8>())
                _JAVM_LDC_INSTRUCTION(LDC_W, BE(frame.ReadCode<u16>()))
                _JAVM_LDC_INSTRUCTION(LDC2_W, BE(frame.ReadCode<u16>()))
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(ILOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(LLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(FLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(DLOAD)
                _JAVM_LOAD_INSTRUCTION_INDEX_READ(ALOAD)
                _JAVM_LOAD_INSTRUCTION(ILOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(FLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(LLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(DLOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(ALOAD_0, 0)
                _JAVM_LOAD_INSTRUCTION(ILOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(FLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(LLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(DLOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(ALOAD_1, 1)
                _JAVM_LOAD_INSTRUCTION(ILOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(FLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(LLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(DLOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(ALOAD_2, 2)
                _JAVM_LOAD_INSTRUCTION(ILOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(FLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(LLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(DLOAD_3, 3)
                _JAVM_LOAD_INSTRUCTION(ALOAD_3, 3)
                _JAVM_ALOAD_INSTRUCTION(IALOAD)
                _JAVM_ALOAD_INSTRUCTION(LALOAD)
                _JAVM_ALOAD_INSTRUCTION(FALOAD)
                _JAVM_ALOAD_INSTRUCTION(DALOAD)
                _JAVM_ALOAD_INSTRUCTION(AALOAD)
                _JAVM_ALOAD_INSTRUCTION(BALOAD)
                _JAVM_ALOAD_INSTRUCTION(CALOAD)
                _JAVM_ALOAD_INSTRUCTION(SALOAD)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(ISTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(LSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(FSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(DSTORE)
                _JAVM_STORE_INSTRUCTION_INDEX_READ(ASTORE)
                _JAVM_STORE_INSTRUCTION(ISTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(FSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(LSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(DSTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(ASTORE_0, 0)
                _JAVM_STORE_INSTRUCTION(ISTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(FSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(LSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(DSTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(ASTORE_1, 1)
                _JAVM_STORE_INSTRUCTION(ISTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(FSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(LSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(DSTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(ASTORE_2, 2)
                _JAVM_STORE_INSTRUCTION(ISTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(FSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(LSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(DSTORE_3, 3)
                _JAVM_STORE_INSTRUCTION(ASTORE_3, 3)
                _JAVM_ASTORE_INSTRUCTION(IASTORE)
                _JAVM_ASTORE_INSTRUCTION(LASTORE)
                _JAVM_ASTORE_INSTRUCTION(FASTORE)
                _JAVM_ASTORE_INSTRUCTION(DASTORE)
                _JAVM_ASTORE_INSTRUCTION(AASTORE)
                _JAVM_ASTORE_INSTRUCTION(BASTORE)
                _JAVM_ASTORE_INSTRUCTION(CASTORE)
                _JAVM_ASTORE_INSTRUCTION(SASTORE)
                case Instruction::POP: {
                    frame.PopStack();
                    break;
                }
                case Instruction::POP2: {
                    if(frame.PopStack()->IsBigComputationalType()) {
                        break;
                    }
                    frame.PopStack();
                    break;
                }
                case Instruction::DUP: {
                    auto var = frame.PopStack();
                    frame.PushStack(var);
                    frame.PushStack(var);
                    break;
                }
                case Instruction::DUP_X1: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    frame.PushStack(var1);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP_X2: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        frame.PushStack(var1);
                        frame.PushStack(var2);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var3 = frame.PopStack();
                    frame.PushStack(var1);
                    frame.PushStack(var3);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2: {
                    auto var1 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        // Special case for long/double
                        frame.PushStack(var1);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var2 = frame.PopStack();
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2_X1: {
                    auto var1 = frame.PopStack();
                    auto var2 = frame.PopStack();
                    if(var1->IsBigComputationalType()) {
                        frame.PushStack(var1);
                        frame.PushStack(var2);
                        frame.PushStack(var1);
                        break;
                    }
                    auto var3 = frame.PopStack();
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    frame.PushStack(var3);
                    frame.PushStack(var2);
                    frame.PushStack(var1);
                    break;
                }
                case Instruction::DUP2_X2: {
                    auto v1 = frame.PopStack();
                    auto v2 = frame.PopStack();
                    auto v3 = frame.PopStack();
                    if(v1->IsBigComputationalType()) {
                        frame.PushStack(v1);
                        frame.PushStack(v3);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    else if(v3->IsBigComputationalType()) {
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        frame.PushStack(v3);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    else if(v2->IsBigComputationalType()) {
                        frame.PushStack(v1);
                        frame.PushStack(v2);
                        frame.PushStack(v1);
                        break;
                    }
                    auto v4 = frame.PopStack();
                    frame.PushStack(v2);
                    frame.PushStack(v1);
                    frame.PushStack(v4);
                    frame.PushStack(v3);
                    frame.PushStack(v2);
                    frame.PushStack(v1);
                    break;
                }
                _JAVM_OPERATOR_INSTRUCTION(IADD, type::Integer, +)
                _JAVM_OPERATOR_INSTRUCTION(LADD, type::Long, +)
                _JAVM_OPERATOR_INSTRUCTION(FADD, type::Float, +)
                _JAVM_OPERATOR_INSTRUCTION(DADD, type::Double, +)
                _JAVM_OPERATOR_INSTRUCTION(ISUB, type::Integer, -)
                _JAVM_OPERATOR_INSTRUCTION(LSUB, type::Long, -)
                _JAVM_OPERATOR_INSTRUCTION(FSUB, type::Float, -)
                _JAVM_OPERATOR_INSTRUCTION(DSUB, type::Double, -)
                _JAVM_OPERATOR_INSTRUCTION(IMUL, type::Integer, *)
                _JAVM_OPERATOR_INSTRUCTION(LMUL, type::Long, *)
                _JAVM_OPERATOR_INSTRUCTION(FMUL, type::Float, *)
                _JAVM_OPERATOR_INSTRUCTION(DMUL, type::Double, *)
                _JAVM_DIV_INSTRUCTION(IDIV, type::Integer)
                _JAVM_DIV_INSTRUCTION(LDIV, type::Long)
                _JAVM_DIV_INSTRUCTION(FDIV, type::Float)
                _JAVM_DIV_INSTRUCTION(DDIV, type::Double)
                _JAVM_OPERATOR_INSTRUCTION(IREM, type::Integer, %)
                _JAVM_OPERATOR_INSTRUCTION(LREM, type::Long, %)
                // Fuck u, decimals
                case Instruction::FREM: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Float>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Float>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Float>(fmodf(var1_val, var2_val));
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::DREM: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Double>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Double>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Double>(fmod(var1_val, var2_val));
                    frame.PushStack(res_var);
                    break;
                }
                _JAVM_NEG_INSTRUCTION(INEG, type::Integer)
                _JAVM_NEG_INSTRUCTION(LNEG, type::Long)
                _JAVM_NEG_INSTRUCTION(FNEG, type::Float)
                _JAVM_NEG_INSTRUCTION(DNEG, type::Double)
                case Instruction::ISHL: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x1F;
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(var1_val << tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::LSHL: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x3F;
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Long>(var1_val << tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::ISHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x1F;
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(var1_val >> tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::LSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x3F;
                    auto res_var = TypeUtils::NewPrimitiveVariable<type::Long>(var1_val >> tmp);
                    frame.PushStack(res_var);
                    break;
                }
                case Instruction::IUSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Integer>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x1F;
                    if(var1_val >= 0) {
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>(var1_val >> tmp);
                        frame.PushStack(res_var);
                    }
                    else {
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Integer>((var1_val >> tmp) + (2 << ~tmp));
                        frame.PushStack(res_var);
                    }
                    break;
                }
                case Instruction::LUSHR: {
                    auto var2 = frame.PopStack();
                    auto var2_obj = var2->GetAs<type::Integer>();
                    auto var2_val = PtrUtils::GetValue(var2_obj);
                    auto var1 = frame.PopStack();
                    auto var1_obj = var1->GetAs<type::Long>();
                    auto var1_val = PtrUtils::GetValue(var1_obj);
                    auto tmp = var2_val & 0x3F;
                    if(var1_val >= 0) {
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Long>(var1_val >> tmp);
                        frame.PushStack(res_var);
                    }
                    else {
                        auto res_var = TypeUtils::NewPrimitiveVariable<type::Long>((var1_val >> tmp) + (2 << ~tmp));
                        frame.PushStack(res_var);
                    }
                    break;
                }
                _JAVM_OPERATOR_INSTRUCTION(IAND, type::Integer, &)
                _JAVM_OPERATOR_INSTRUCTION(LAND, type::Long, &)
                _JAVM_OPERATOR_INSTRUCTION(IOR, type::Integer, |)
                _JAVM_OPERATOR_INSTRUCTION(LOR, type::Long, |)
                _JAVM_OPERATOR_INSTRUCTION(IXOR, type::Integer, ^)
                _JAVM_OPERATOR_INSTRUCTION(LXOR, type::Long, ^)
                case Instruction::IINC: {
                    u8 idx = frame.ReadCode<u8>();
                    i8 cnst = frame.ReadCode<i8>();
                    auto var = frame.GetLocalAt((u32)idx);
                    auto var_obj = var->GetAs<type::Integer>();
                    auto var_val = PtrUtils::GetValue(var_obj);
                    var_val += cnst;
                    auto new_var = TypeUtils::NewPrimitiveVariable<type::Integer>(var_val);
                    frame.SetLocalAt((u32)idx, new_var);
                    break;
                }
                _JAVM_CONVERSION_INSTRUCTION(I2L, type::Integer, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(I2F, type::Integer, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(I2D, type::Integer, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(L2I, type::Long, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(L2F, type::Long, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(L2D, type::Long, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(F2I, type::Float, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(F2L, type::Float, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(F2D, type::Float, type::Double)
                _JAVM_CONVERSION_INSTRUCTION(D2I, type::Double, type::Integer)
                _JAVM_CONVERSION_INSTRUCTION(D2L, type::Double, type::Long)
                _JAVM_CONVERSION_INSTRUCTION(D2F, type::Double, type::Float)
                _JAVM_CONVERSION_INSTRUCTION(I2B, type::Integer, type::Byte)
                _JAVM_CONVERSION_INSTRUCTION(I2C, type::Integer, type::Character)
                _JAVM_CONVERSION_INSTRUCTION(I2S, type::Integer, type::Short)
                _JAVM_CMP_INSTRUCTION(LCMP, type::Long)
                _JAVM_FDCMP_INSTRUCTION(FCMPL, type::Float, -1)
                _JAVM_FDCMP_INSTRUCTION(FCMPG, type::Float, 1)
                _JAVM_FDCMP_INSTRUCTION(DCMPL, type::Double, -1)
                _JAVM_FDCMP_INSTRUCTION(DCMPG, type::Double, 1)

                #define _JAVM_IF_INSTRUCTION(op) \
                i16 branch = BE(frame.ReadCode<i16>()); \
                auto var = frame.PopStack(); \
                auto var_val = var->GetValue<type::Integer>(); \
                if(var_val op 0) { \
                    pos -= 3; \
                    pos += branch; \
                } \

                case Instruction::IFEQ: {
                    _JAVM_IF_INSTRUCTION(==)
                    break;
                }
                case Instruction::IFNE: {
                    _JAVM_IF_INSTRUCTION(!=)
                    break;
                }
                case Instruction::IFLT: {
                    _JAVM_IF_INSTRUCTION(<)
                    break;
                }
                case Instruction::IFGE: {
                    _JAVM_IF_INSTRUCTION(>=)
                    break;
                }
                case Instruction::IFGT: {
                    _JAVM_IF_INSTRUCTION(>)
                    break;
                }
                case Instruction::IFLE: {
                    _JAVM_IF_INSTRUCTION(<=)
                    break;
                }

                #define _JAVM_ICMP_INSTRUCTION(op) \
                i16 branch = BE(frame.ReadCode<i16>()); \
                auto var2 = frame.PopStack(); \
                auto var2_val = var2->GetValue<type::Integer>(); \
                auto var1 = frame.PopStack(); \
                auto var1_val = var1->GetValue<type::Integer>(); \
                JAVM_LOG("[icmp] %d " #op " %d", var1_val, var2_val); \
                if(var1_val op var2_val) { \
                    pos -= 3; \
                    pos += branch; \
                } \

                case Instruction::IF_ICMPEQ: {
                    _JAVM_ICMP_INSTRUCTION(==)
                    break;
                }
                case Instruction::IF_ICMPNE: {
                    _JAVM_ICMP_INSTRUCTION(!=)
                    break;
                }
                case Instruction::IF_ICMPLT: {
                    _JAVM_ICMP_INSTRUCTION(<)
                    break;
                }
                case Instruction::IF_ICMPGE: {
                    _JAVM_ICMP_INSTRUCTION(>=)
                    break;
                }
                case Instruction::IF_ICMPGT: {
                    _JAVM_ICMP_INSTRUCTION(>)
                    break;
                }
                case Instruction::IF_ICMPLE: {
                    _JAVM_ICMP_INSTRUCTION(<=)
                    break;
                }
                case Instruction::IF_ACMPEQ: {
                    auto var2 = frame.PopStack();
                    auto var1 = frame.PopStack();
                    JAVM_LOG("[acmpeq] %s == %s", StrUtils::ToUtf8(TypeUtils::FormatVariable(var1)).c_str(), StrUtils::ToUtf8(TypeUtils::FormatVariable(var2)).c_str());
                    i16 branch = BE(frame.ReadCode<i16>());
                    if(PtrUtils::Equal(var1, var2)) {
                        pos -= 3;
                        pos += branch;
                    }
                    break;
                }
                case Instruction::IF_ACMPNE: {
                    auto var2 = frame.PopStack();
                    auto var1 = frame.PopStack();
                    JAVM_LOG("[acmpne] %s != %s", StrUtils::ToUtf8(TypeUtils::FormatVariable(var1)).c_str(), StrUtils::ToUtf8(TypeUtils::FormatVariable(var2)).c_str());
                    i16 branch = BE(frame.ReadCode<i16>());
                    if(!PtrUtils::Equal(var1, var2)) {
                        pos -= 3;
                        pos += branch;
                    }
                    break;
                }
                case Instruction::GOTO: {
                    i16 branch = BE(frame.ReadCode<i16>());
                    pos -= 3;
                    pos += branch;
                    break;
                }
                case Instruction::JSR:
                case Instruction::RET: {
                    // Old instructions, not implementing them
                    break;
                }
                // TODO: TABLESWITCH
                case Instruction::LOOKUPSWITCH: {
                    auto prev_pos = pos - 1;
                    auto orig_prev_pos = prev_pos;
                    if((prev_pos % 4) != 0) {
                        prev_pos += (4 - prev_pos % 4);
                    }
                    else {
                        prev_pos += 4;
                    }
                    auto orig_pos = pos;
                    // Change position
                    pos = prev_pos;
                    auto default_byte = BE(frame.ReadCode<u32>());
                    auto count = BE(frame.ReadCode<u32>());
                    std::map<u32, u32> table;
                    for(u32 i = 0; i < count; i++) {
                        auto value = BE(frame.ReadCode<u32>());
                        auto val_pos = BE(frame.ReadCode<u32>());
                        table.insert(std::make_pair(value, val_pos));
                    }
                    // Restore position
                    pos = orig_pos;
                    auto top_v = frame.PopStack();
                    auto top = top_v->GetValue<type::Integer>();
                    auto it = table.find(top);
                    if(it != table.end()) {
                        pos = it->second + orig_prev_pos;
                    }
                    else {
                        pos = default_byte + orig_prev_pos;
                    }
                    break;
                }
                case Instruction::IRETURN:
                case Instruction::LRETURN:
                case Instruction::FRETURN:
                case Instruction::DRETURN:
                case Instruction::ARETURN: {
                    auto var = frame.PopStack();
                    JAVM_LOG("[*return] Returning '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str());
                    if(var->CanGetAs<VariableType::Integer>()) {
                        JAVM_LOG("[*return] Return int value: %d", var->GetValue<type::Integer>());
                    }
                    if(var->CanGetAs<VariableType::Boolean>()) {
                        JAVM_LOG("[*return] Return boolean value: %s", var->GetValue<type::Boolean>() ? "true" : "false");
                    }
                    return ExecutionResult::ReturnVariable(var);
                }
                case Instruction::RETURN: {
                    // Return nothing (void)
                    JAVM_LOG("[return] Returning void...");
                    return ExecutionResult::Void();
                }
                case Instruction::GETSTATIC: {
                    u16 index = BE(frame.ReadCode<u16>());

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                    if(const_field_item) {
                        auto const_field_data = const_field_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            auto class_name = const_class_data.processed_name;
                            auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(field_nat_item) {
                                auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                auto field_name = field_nat_data.processed_name;
                                auto field_desc = field_nat_data.processed_desc;

                                JAVM_LOG("[getstatic] Get static field '%s' ('%s') of '%s'...", StrUtils::ToUtf8(field_name).c_str(), StrUtils::ToUtf8(field_desc).c_str(), StrUtils::ToUtf8(class_name).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    auto var = class_type->GetStaticField(field_name, field_desc);
                                    JAVM_LOG("[getstatic] Static field: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str());
                                    frame.PushStack(var);
                                }
                                else {
                                    JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                }
                            }
                            else {
                                JAVM_LOG("Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item Class...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                case Instruction::PUTSTATIC: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                    if(const_field_item) {
                        auto const_field_data = const_field_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            auto class_name = const_class_data.processed_name;
                            auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(field_nat_item) {
                                auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                auto field_name = field_nat_data.processed_name;
                                auto field_desc = field_nat_data.processed_desc;

                                JAVM_LOG("[putstatic] Set static field '%s' ('%s') of '%s'...", StrUtils::ToUtf8(field_name).c_str(), StrUtils::ToUtf8(field_desc).c_str(), StrUtils::ToUtf8(class_name).c_str());
                                JAVM_LOG("[putstatic] Static field: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(var)).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    class_type->SetStaticField(field_name, field_desc, var);
                                }
                                else {
                                    JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                }
                            }
                            else {
                                JAVM_LOG("Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item Class...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                case Instruction::GETFIELD: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();

                        auto &const_pool = frame.GetThisConstantPool();
                        auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                        if(const_field_item) {
                            auto const_field_data = const_field_item->GetFieldMethodRefData();
                            auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                            if(const_class_item) {
                                auto const_class_data = const_class_item->GetClassData();
                                auto class_name = const_class_data.processed_name;
                                auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                                if(field_nat_item) {
                                    auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                    auto field_name = field_nat_data.processed_name;
                                    auto field_desc = field_nat_data.processed_desc;

                                    JAVM_LOG("[getfield] Get field '%s' ('%s') of '%s'...", StrUtils::ToUtf8(field_name).c_str(), StrUtils::ToUtf8(field_desc).c_str(), StrUtils::ToUtf8(class_name).c_str());

                                    auto var_obj_c = var_obj->GetInstanceByClassType(var_obj, class_name);
                                    if(var_obj_c) {
                                        auto field_var = var_obj_c->GetField(field_name, field_desc);
                                        JAVM_LOG("[getfield] Field: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(field_var)).c_str());
                                        frame.PushStack(field_var);
                                    }
                                    else {
                                        JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                    }
                                }
                                else {
                                    JAVM_LOG("Invalid const pool item NAT...?");
                                }
                            }
                            else {
                                JAVM_LOG("Invalid const pool item Class...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item FieldRef...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid class object...");
                    }

                    break;
                }
                case Instruction::PUTFIELD: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto field_var = frame.PopStack();
                    auto var = frame.PopStack();

                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();

                        auto &const_pool = frame.GetThisConstantPool();
                        auto const_field_item = const_pool.GetItemAt(index, ConstantPoolTag::FieldRef);
                        if(const_field_item) {
                            auto const_field_data = const_field_item->GetFieldMethodRefData();
                            auto const_class_item = const_pool.GetItemAt(const_field_data.class_index, ConstantPoolTag::Class);
                            if(const_class_item) {
                                auto const_class_data = const_class_item->GetClassData();
                                auto class_name = const_class_data.processed_name;
                                auto field_nat_item = const_pool.GetItemAt(const_field_data.name_and_type_index, ConstantPoolTag::NameAndType);
                                if(field_nat_item) {
                                    auto field_nat_data = field_nat_item->GetNameAndTypeData();
                                    auto field_name = field_nat_data.processed_name;
                                    auto field_desc = field_nat_data.processed_desc;

                                    JAVM_LOG("[putfield] Set field '%s' ('%s') of '%s'...", StrUtils::ToUtf8(field_name).c_str(), StrUtils::ToUtf8(field_desc).c_str(), StrUtils::ToUtf8(class_name).c_str());
                                    JAVM_LOG("[putfield] Field: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(field_var)).c_str());

                                    auto var_obj_c = var_obj->GetInstanceByClassType(var_obj, class_name);
                                    if(var_obj_c) {
                                        var_obj_c->SetField(field_name, field_desc, field_var);
                                    }
                                    else {
                                        JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                    }
                                }
                                else {
                                    JAVM_LOG("Invalid const pool item NAT...?");
                                }
                            }
                            else {
                                JAVM_LOG("Invalid const pool item Class...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item FieldRef...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid class object...");
                    }

                    break;
                }
                case Instruction::INVOKEVIRTUAL:
                case Instruction::INVOKESPECIAL:
                case Instruction::INVOKEINTERFACE: {
                    u16 index = BE(frame.ReadCode<u16>());
                    const bool is_interface = inst == Instruction::INVOKEINTERFACE;
                    const bool is_virtual = inst == Instruction::INVOKEVIRTUAL;
                    const bool is_special = inst == Instruction::INVOKESPECIAL;
                    if(is_interface) {
                        // Interface has extra items, both unused...?
                        u8 count = frame.ReadCode<u8>();
                        u8 zero = frame.ReadCode<u8>();
                    }
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_fn_item = const_pool.GetItemAt(index, is_interface ? ConstantPoolTag::InterfaceMethodRef : ConstantPoolTag::MethodRef);
                    if(const_fn_item) {
                        auto const_fn_data = const_fn_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_fn_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            auto class_name = const_class_data.processed_name;
                            auto const_fn_nat_item = const_pool.GetItemAt(const_fn_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(const_fn_nat_item) {
                                auto const_fn_nat_data = const_fn_nat_item->GetNameAndTypeData();
                                auto fn_name = const_fn_nat_data.processed_name;
                                auto fn_desc = const_fn_nat_data.processed_desc;

                                if(is_virtual) {
                                    JAVM_LOG("[invokevirtual]");
                                }
                                if(is_special) {
                                    JAVM_LOG("[invokespecial]");
                                }
                                if(is_interface) {
                                    JAVM_LOG("[invokeinterface]");
                                }

                                JAVM_LOG("[invoke] Executing '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());

                                auto [this_var, param_vars] = ExecutionUtils::LoadInstanceMethodParameters(frame, fn_desc);
                                JAVM_LOG("[invoke] Parameter count: %ld + this...", param_vars.size());
                                if(this_var->CanGetAs<VariableType::ClassInstance>()) {
                                    auto this_var_obj = this_var->GetAs<type::ClassInstance>();
                                    Ptr<ClassInstance> this_var_obj_c = nullptr;
                                    if(is_special) {
                                        this_var_obj_c = this_var_obj->GetInstanceByClassTypeAndMethodSpecial(this_var_obj, class_name, fn_name, fn_desc);
                                    }
                                    else {
                                        this_var_obj_c = this_var_obj->GetInstanceByClassTypeAndMethodVirtualInterface(this_var_obj, class_name, fn_name, fn_desc);
                                    }
                                    JAVM_LOG("[invoke] This type: '%s'...", StrUtils::ToUtf8(this_var_obj->GetClassType()->GetClassName()).c_str());
                                    JAVM_LOG("[invoke] Detected instance type: '%s'...", StrUtils::ToUtf8(this_var_obj_c->GetClassType()->GetClassName()).c_str());
                                    if(this_var_obj_c) {
                                        auto result = this_var_obj_c->CallInstanceMethod(fn_name, fn_desc, this_var, param_vars);
                                        if(result.IsInvalidOrThrown()) {
                                            JAVM_LOG("Invalid/thrown execution of '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());
                                            return result;
                                        }
                                        if(result.Is<ExecutionStatus::VariableReturn>()) {
                                            auto ret_var = result.ret_var;
                                            if(ret_var) {
                                                frame.PushStack(ret_var);
                                            }
                                            else {
                                                JAVM_LOG("Invalid method return value...");
                                            }
                                        }
                                    }
                                    else {
                                        JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                    }
                                }
                                else {
                                    if(this_var->IsNull()) {
                                        JAVM_LOG("Invalid class object - is null...");
                                    }
                                    else {
                                        JAVM_LOG("Invalid class object...");
                                    }
                                }
                                JAVM_LOG("[invoke] Done '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());
                            }
                            else {
                                JAVM_LOG("Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item Class...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item FieldRef...? - id: %d, count: %ld", index, const_pool.GetItemCount());
                    }

                    break;
                }
                case Instruction::INVOKESTATIC: {
                    u16 index = BE(frame.ReadCode<u16>());
                    
                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_fn_item = const_pool.GetItemAt(index, ConstantPoolTag::MethodRef);
                    if(const_fn_item) {
                        auto const_fn_data = const_fn_item->GetFieldMethodRefData();
                        auto const_class_item = const_pool.GetItemAt(const_fn_data.class_index, ConstantPoolTag::Class);
                        if(const_class_item) {
                            auto const_class_data = const_class_item->GetClassData();
                            auto class_name = const_class_data.processed_name;
                            auto const_fn_nat_item = const_pool.GetItemAt(const_fn_data.name_and_type_index, ConstantPoolTag::NameAndType);
                            if(const_fn_nat_item) {
                                auto const_fn_nat_data = const_fn_nat_item->GetNameAndTypeData();
                                auto fn_name = const_fn_nat_data.processed_name;
                                auto fn_desc = const_fn_nat_data.processed_desc;

                                JAVM_LOG("[invokestatic] Executing '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());

                                auto class_type = rt::LocateClassType(class_name);
                                if(class_type) {
                                    auto param_vars = ExecutionUtils::LoadClassMethodParameters(frame, fn_desc);
                                    JAVM_LOG("[invokestatic] Parameter count: %ld...", param_vars.size());
                                    auto result = class_type->CallClassMethod(fn_name, fn_desc, param_vars);
                                    if(result.IsInvalidOrThrown()) {
                                        JAVM_LOG("Invalid/thrown execution of '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());
                                        return result;
                                    }
                                    if(result.Is<ExecutionStatus::VariableReturn>()) {
                                        auto ret_var = result.ret_var;
                                        if(ret_var) {
                                            frame.PushStack(ret_var);
                                        }
                                        else {
                                            JAVM_LOG("Invalid instance method return value...");
                                        }
                                    }
                                }
                                else {
                                    JAVM_LOG("Invalid class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                                }

                                JAVM_LOG("[invokestatic] Done '%s'::'%s'::'%s'...", StrUtils::ToUtf8(class_name).c_str(), StrUtils::ToUtf8(fn_name).c_str(), StrUtils::ToUtf8(fn_desc).c_str());
                            }
                            else {
                                JAVM_LOG("Invalid const pool item NAT...?");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid const pool item Class...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item FieldRef...?");
                    }

                    break;
                }
                // TODO: INVOKEDYNAMIC
                case Instruction::NEW: {
                    u16 index = BE(frame.ReadCode<u16>());

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[NEW] new '%s'...", StrUtils::ToUtf8(class_name).c_str());

                        auto class_type = rt::LocateClassType(class_name);
                        if(class_type) {
                            auto ret = class_type->EnsureStaticInitializerCalled();
                            if(ret.IsInvalidOrThrown()) {
                                return ret;
                            }
                            auto class_var = TypeUtils::NewClassVariable(class_type);
                            frame.PushStack(class_var);
                        }
                        else {
                            JAVM_LOG("Invalid class type...?");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool class item...?");
                    }

                    break;
                }
                case Instruction::NEWARRAY: {
                    u8 type = frame.ReadCode<u8>();
                    auto len_var = frame.PopStack();

                    if(len_var->CanGetAs<VariableType::Integer>()) {
                        auto len_val = len_var->GetValue<type::Integer>();
                        if(len_val >= 0) {
                            auto val_type = GetVariableTypeFromNewArrayType(static_cast<NewArrayType>(type));
                            if(val_type != VariableType::Invalid) {
                                auto arr_val = TypeUtils::NewArray(len_val, val_type);
                                frame.PushStack(arr_val);
                            }
                            else {
                                JAVM_LOG("Invalid array type...");
                            }
                        }
                        else {
                            JAVM_LOG("Array length < 0...");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid length variable...");
                    }

                    break;
                }
                case Instruction::ANEWARRAY: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto len_var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[anewarray] Class name: '%s'", StrUtils::ToUtf8(class_name).c_str());
                        auto class_type = rt::LocateClassType(class_name);
                        JAVM_LOG("[anewarray] Got class: '%s'", StrUtils::ToUtf8(class_name).c_str());
                        if(class_type) {
                            auto ret = class_type->EnsureStaticInitializerCalled();
                            if(ret.IsInvalidOrThrown()) {
                                return ret;
                            }
                            if(len_var->CanGetAs<VariableType::Integer>()) {
                                auto len_val = len_var->GetValue<type::Integer>();
                                if(len_val >= 0) {
                                    JAVM_LOG("[anewarray] Array length: %d", len_val);
                                    auto arr_var = TypeUtils::NewArray(len_val, class_type);
                                    JAVM_LOG("[anewarray] Created array! '%s'", StrUtils::ToUtf8(TypeUtils::FormatVariableType(arr_var)).c_str());
                                    frame.PushStack(arr_var);
                                }
                                else {
                                    JAVM_LOG("Array length < 0: %d", len_val);
                                }
                            }
                            else {
                                JAVM_LOG("Invalid length variable...");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid array class type...");
                        }
                    }
                    else {
                        JAVM_LOG("Invalid constant pool item...");
                    }

                    break;
                }
                case Instruction::ARRAYLENGTH: {
                    
                    auto arr_var = frame.PopStack();
                    if(arr_var->CanGetAs<VariableType::Array>()) {
                        auto arr_obj = arr_var->GetAs<type::Array>();
                        
                        auto arr_len = arr_obj->GetLength();

                        JAVM_LOG("[arraylength] -> Array: '%s'...", StrUtils::ToUtf8(TypeUtils::FormatVariableType(arr_var)).c_str());
                        JAVM_LOG("[arraylength] -> Length: %d...", arr_obj->GetLength());

                        auto len_var = TypeUtils::NewPrimitiveVariable<type::Integer>(arr_len);
                        frame.PushStack(len_var);
                    }
                    else {
                        JAVM_LOG("Invalid array object...");
                    }

                    break;
                }
                case Instruction::ATHROW: {
                    // TODO - proper exception handling
                    auto throwable_var = frame.PopStack();
                    JAVM_LOG("[athrow] Throwable object: '%s'", StrUtils::ToUtf8(TypeUtils::FormatVariableType(throwable_var)).c_str());
                    auto throwable_obj = throwable_var->GetAs<type::ClassInstance>();
                    auto msg = throwable_obj->CallInstanceMethod(u"getMessage", u"()Ljava/lang/String;", throwable_var);
                    auto msg_str = StringUtils::GetValue(msg.ret_var);
                    JAVM_LOG("[athrow] Thrown message: '%s'", StrUtils::ToUtf8(msg_str).c_str());
                    inner_impl::NotifyExceptionThrownImpl(throwable_var);
                    return ExecutionResult::Thrown();
                }
                case Instruction::CHECKCAST: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        auto class_name = const_class_data.processed_name;

                        JAVM_LOG("[checkcast] class name: '%s'", StrUtils::ToUtf8(class_name).c_str());

                        if(var->CanGetAs<VariableType::ClassInstance>()) {
                            auto var_obj = var->GetAs<type::ClassInstance>();
                            JAVM_LOG("[checkcast] instance class name: '%s'", StrUtils::ToUtf8(var_obj->GetClassType()->GetClassName()).c_str());
                            if(var_obj->GetClassType()->CanCastTo(class_name)) {
                                frame.PushStack(var);
                            }
                            else {
                                JAVM_LOG("-exception- invalid object cast...");
                            }
                        }
                        else if(var->CanGetAs<VariableType::NullObject>()) {
                            frame.PushStack(var);
                        }
                        else if(var->CanGetAs<VariableType::Array>()) {
                            auto arr_obj = var->GetAs<type::Array>();
                            auto class_type = arr_obj->GetClassType();
                            if(class_type->CanCastTo(TypeUtils::GetBaseClassName(class_name))) {
                                frame.PushStack(var);
                            }
                            else {
                                JAVM_LOG("-exception- invalid array cast...");
                            }
                        }
                        else {
                            JAVM_LOG("Invalid var type: %d", static_cast<u32>(var->GetType()));
                        }
                    }
                    else {
                        JAVM_LOG("Invalid const pool item");
                    }

                    break;
                }
                case Instruction::INSTANCEOF: {
                    u16 index = BE(frame.ReadCode<u16>());
                    auto var = frame.PopStack();

                    auto &const_pool = frame.GetThisConstantPool();
                    auto const_class_item = const_pool.GetItemAt(index, ConstantPoolTag::Class);
                    if(const_class_item) {
                        auto const_class_data = const_class_item->GetClassData();
                        auto class_name = const_class_data.processed_name;

                        if(var->CanGetAs<VariableType::ClassInstance>()) {
                            auto var_obj = var->GetAs<type::ClassInstance>();
                            if(var_obj->GetClassType()->CanCastTo(class_name)) {
                                auto ret_var = TypeUtils::NewPrimitiveVariable<type::Integer>(1);
                                frame.PushStack(ret_var);
                            }
                            else {
                                auto ret_var = TypeUtils::NewPrimitiveVariable<type::Integer>(0);
                                frame.PushStack(ret_var);
                            }
                        }
                        else if(var->CanGetAs<VariableType::NullObject>()) {
                            auto ret_var = TypeUtils::NewPrimitiveVariable<type::Integer>(0);
                            frame.PushStack(ret_var);
                        }
                    }

                    break;
                }
                case Instruction::MONITORENTER: {
                    auto var = frame.PopStack();
                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();
                        auto monitor = var_obj->GetMonitor();
                        monitor->Enter();
                    }
                    else {
                        return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid monitor enter");
                    }

                    break;
                }
                case Instruction::MONITOREXIT: {
                    auto var = frame.PopStack();
                    if(var->CanGetAs<VariableType::ClassInstance>()) {
                        auto var_obj = var->GetAs<type::ClassInstance>();
                        auto monitor = var_obj->GetMonitor();
                        monitor->Leave();
                    }
                    else {
                        return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid monitor leave");
                    }

                    break;
                }
                case Instruction::IFNULL: {
                   auto var = frame.PopStack();
                    i16 branch = BE(frame.ReadCode<i16>());
                    if(var->CanGetAs<VariableType::NullObject>()) {
                        pos -= 3;
                        pos += branch;
                    }
                    break;
                }
                case Instruction::IFNONNULL: {
                    auto var = frame.PopStack();
                    i16 branch = BE(frame.ReadCode<i16>());
                    if(!var->CanGetAs<VariableType::NullObject>()) {
                        pos -= 3;
                        pos += branch;
                    }
                    break;
                }
                case Instruction::GOTO_W:
                case Instruction::JSR_W: {
                    // Old instructions, not implementing them
                    break;
                }
                
                default:
                    return ExceptionUtils::ThrowWithTypeAndMessage(u"java/lang/Exception", u"Invalid or unimplemented instruction: " + StrUtils::From(static_cast<u32>(inst)));
            }
            return ExecutionResult::ContinueCodeExecution();
        }

    }

    namespace inner_impl {

        inline void SetLocalParametersImpl(ExecutionFrame &frame, std::vector<Ptr<Variable>> param_vars, u32 i_base) {
            u32 i = i_base;
            for(auto param: param_vars) {
                frame.SetLocalAt(i, param);
                i++;
                if(param->CanGetAs<VariableType::Long>() || param->CanGetAs<VariableType::Double>()) {
                    i++;
                }
            }
        }
        
        inline void SetLocalStaticParameters(ExecutionFrame &frame, std::vector<Ptr<Variable>> param_vars) {
            SetLocalParametersImpl(frame, param_vars, 0);
        }

        inline void SetLocalParameters(ExecutionFrame &frame, Ptr<Variable> this_var, std::vector<Ptr<Variable>> param_vars) {
            frame.SetLocalAt(0, this_var);
            SetLocalParametersImpl(frame, param_vars, 1);
        }

        ExecutionResult CommonDoExecute(ExecutionFrame &frame) {
            // First of all, check if any exceptions were thrown in any threads
            if(inner_impl::WasExceptionThrownImpl()) {
                JAVM_LOG("[execution] Exception thrown, skipping execution...");
                return ExecutionResult::Thrown();
            }

            while(true) {
                auto res = exec_impl::HandleInstructionImpl(frame);
                if(!res.Is<ExecutionStatus::ContinueExecution>()) {
                    return res;
                }

            }
            // This will NEVER be reached...
        }

        template<typename ...JArgs>
        ExecutionResult ExecuteStaticCode(u8 *code_ptr, u16 max_locals, ConstantPool pool, std::vector<Ptr<Variable>> param_vars) {
            auto max_locals_val = max_locals;
            for(auto param: param_vars) {
                // Longs and doubles take extra spaces
                if(param->CanGetAs<VariableType::Long>() || param->CanGetAs<VariableType::Double>()) {
                    max_locals_val++;
                }
            }
            ExecutionFrame frame(code_ptr, max_locals_val, pool);
            SetLocalStaticParameters(frame, param_vars);
            return CommonDoExecute(frame);
        }

        template<typename ...JArgs>
        inline ExecutionResult ExecuteStaticCode(u8 *code_ptr, u16 max_locals, ConstantPool pool, JArgs &&...java_args) {
            std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
            return ExecuteStaticCode(code_ptr, max_locals, pool, param_vars);
        }

        template<typename ...JArgs>
        ExecutionResult ExecuteCode(u8 *code_ptr, u16 max_locals, Ptr<Variable> this_var, ConstantPool pool, std::vector<Ptr<Variable>> param_vars) {
            auto max_locals_val = max_locals;
            for(auto param: param_vars) {
                // Longs and doubles take extra spaces
                if(param->CanGetAs<VariableType::Long>() || param->CanGetAs<VariableType::Double>()) {
                    max_locals_val++;
                }
            }
            ExecutionFrame frame(code_ptr, max_locals_val, pool, this_var);
            SetLocalParameters(frame, this_var, param_vars);
            return CommonDoExecute(frame);
        }

        template<typename ...JArgs>
        inline ExecutionResult ExecuteCode(u8 *code_ptr, u16 max_locals, Ptr<Variable> this_var, ConstantPool pool, JArgs &&...java_args) {
            std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
            return ExecuteCode(code_ptr, max_locals, this_var, pool, param_vars);
        }

    }

}