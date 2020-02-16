
#pragma once
#include <javm/core/core_Instructions.hpp>
#include <javm/core/core_Archive.hpp>
#include <javm/core/core_CodeAttribute.hpp>
#include <javm/native/native_Class.hpp>
#include <javm/native/native_Standard.hpp>
#include <algorithm>
#include <memory>
#include <cmath>

namespace javm::core {

    struct ThrownInfo {

        bool info_valid;
        std::string class_type;
        std::string message;

        // TODO: more fields

    };

    template<bool CallCtor, typename ...Args>
    Value CreateNewClassWith(void *machine, const std::string &name, std::function<void(ClassObject*)> ref_fn, Args &&...args);

    class Machine {

        private:
            std::shared_ptr<ClassObject> empty_class;
            std::vector<std::shared_ptr<Archive>> loaded_archives;
            std::vector<std::shared_ptr<ClassObject>> class_files;
            std::vector<std::shared_ptr<ClassObject>> native_classes;
            std::vector<std::shared_ptr<ClassObject>> native_function_classes;
            ThrownInfo thrown_info;

            void ThrowClassNotFound(const std::string &class_name) {
                this->ThrowWithType("java.lang.NoClassDefFoundError", ClassObject::ProcessClassName(class_name));
            }

            void ThrowRuntimeException(const std::string &message) {
                this->ThrowWithType("java.lang.RuntimeException", message);
            }

            ValueType GetValueTypeFromNewArrayType(NewArrayType type) {
                switch(type) {
                    case NewArrayType::Boolean:
                        return ValueType::Boolean;
                    case NewArrayType::Character:
                        return ValueType::Character;
                    case NewArrayType::Float:
                        return ValueType::Float;
                    case NewArrayType::Double:
                        return ValueType::Double;
                    case NewArrayType::Byte:
                        return ValueType::Byte;
                    case NewArrayType::Short:
                        return ValueType::Short;
                    case NewArrayType::Integer:
                        return ValueType::Integer;
                    case NewArrayType::Long:
                        return ValueType::Long;
                    default:
                        return ValueType::Void;
                }
                return ValueType::Void;
            }

            template<typename Arg>
            void HandlePushArgument(Frame &frame, u32 &index, Arg &arg) {
                static_assert(std::is_same_v<Arg, Value>, "Arguments must be core::Value!");
                frame.SetLocal(index, arg);
                index++;
            }

            Value HandleInstruction(Frame &frame, bool &should_ret, Instruction inst) {
                auto &offset = frame.GetOffset();
                // printf("Instruction: 0x%X\n", (u32)inst);

                #define _JAVM_LOAD_INSTRUCTION(instr, idx) \
                case Instruction::instr: { \
                        u8 index = idx; \
                        auto ref = frame.GetLocal((u32)index); \
                        frame.Push(ref); \
                        break; \
                    }

                #define _JAVM_LOAD_INSTRUCTION_INDEX_READ(instr) _JAVM_LOAD_INSTRUCTION(instr, frame.Read<u8>())
                
                #define _JAVM_STORE_INSTRUCTION(instr, idx) \
                case Instruction::instr: { \
                        u8 index = idx; \
                        auto val = frame.Pop(); \
                        frame.SetLocal((u32)index, val); \
                        break; \
                    }
                
                #define _JAVM_STORE_INSTRUCTION_INDEX_READ(instr) _JAVM_STORE_INSTRUCTION(instr, frame.Read<u8>())

                #define _JAVM_LDC_BASE(idx) \
                auto &constant_pool = frame.GetCurrentClass()->GetConstantPool(); \
                auto &constant = constant_pool[index - 1]; \
                switch(constant.GetTag()) { \
                    case CPTag::Integer: { \
                        int value = constant.GetIntegerData().integer; \
                        frame.CreatePush<int>(value); \
                        break; \
                    } \
                    case CPTag::Float: { \
                        float value = constant.GetFloatData().flt; \
                        frame.CreatePush<float>(value); \
                        break; \
                    } \
                    case CPTag::Long: { \
                        long value = constant.GetLongData().lng; \
                        frame.CreatePush<long>(value); \
                        break; \
                    } \
                    case CPTag::Double: { \
                        double value = constant.GetDoubleData().dbl; \
                        frame.CreatePush<double>(value); \
                        break; \
                    } \
                    case CPTag::String: { \
                        auto value = constant.GetStringData().processed_string; \
                        auto str_obj = CreateNewClassWith<true>(this, "java.lang.String", [&](auto *ref) { \
                            reinterpret_cast<java::lang::String*>(ref)->SetNativeString(value); \
                        }); \
                        frame.Push(str_obj); \
                        break; \
                    } \
                    case CPTag::Class: { \
                        auto class_name = constant.GetClassData().processed_name; \
                        if(this->HasClass(class_name)) { \
                            auto class_def = this->FindClass(class_name); \
                            auto class_value = CreateNewValue<java::lang::Class>(); \
                            auto class_ref = class_value->GetReference<java::lang::Class>(); \
                            class_ref->SetClassDefinition(class_def); \
                            frame.Push(class_value); \
                        } \
                        else { \
                            this->ThrowClassNotFound(class_name); \
                        } \
                        break; \
                    } \
                    default: \
                        break; \
                }

                #define _JAVM_LDC_INSTRUCTION(instr, idx) \
                case Instruction::instr: { \
                        auto index = idx; \
                        _JAVM_LDC_BASE(index) \
                        break; \
                    }

                #define _JAVM_ALOAD_INSTRUCTION(instr) \
                case Instruction::instr: { \
                        int index = frame.PopValue<int>(); \
                        if(index < 0) { \
                            this->ThrowRuntimeException("Invalid array index"); \
                        } \
                        else { \
                            auto value = frame.Pop(); \
                            if(value->IsArray()) { \
                                auto array = value->GetReference<Array>(); \
                                if(array->CheckIndex(index)) { \
                                    auto val = array->GetAt(index); \
                                    frame.Push(val); \
                                } \
                                else { \
                                    this->ThrowRuntimeException("Invalid array index"); \
                                } \
                            } \
                            else { \
                                this->ThrowRuntimeException("Invalid input variable (not an array)"); \
                            } \
                        } \
                        break; \
                    }

                #define _JAVM_ASTORE_INSTRUCTION(instr) \
                case Instruction::instr: { \
                        auto value = frame.Pop(); \
                        int index = frame.PopValue<int>(); \
                        if(index < 0) { \
                            this->ThrowRuntimeException("Invalid array index"); \
                        } \
                        else { \
                            auto arr_value = frame.Pop(); \
                            if(arr_value->IsArray()) { \
                                auto array = arr_value->GetReference<Array>(); \
                                if(array->GetValueType() == value->GetValueType()) { \
                                    if(array->CheckIndex(index)) { \
                                        array->SetAt(index, value); \
                                    } \
                                    else { \
                                        this->ThrowRuntimeException("Invalid array index"); \
                                    } \
                                }  \
                                else { \
                                    this->ThrowRuntimeException("Value and array type mismatch"); \
                                } \
                            } \
                            else { \
                                this->ThrowRuntimeException("Invalid input variable (not an array)"); \
                            } \
                        } \
                        break; \
                    }

                #define _JAVM_OPERATOR_INSTRUCTION(instr, type, op) \
                case Instruction::instr: { \
                        type v2 = frame.PopValue<type>(); \
                        type v1 = frame.PopValue<type>(); \
                        frame.CreatePush<type>(v1 op v2); \
                        break; \
                    }

                #define _JAVM_NEG_INSTRUCTION(instr, type) \
                case Instruction::instr: { \
                        type val = frame.PopValue<type>(); \
                        frame.CreatePush<type>(-val); \
                        break; \
                    }

                #define _JAVM_CONVERSION_INSTRUCTION(instr, t1, t2) \
                case Instruction::instr: { \
                        t1 val = frame.PopValue<t1>(); \
                        frame.CreatePush<t2>((t2)val); \
                        break; \
                    }

                #define _JAVM_CMP_INSTRUCTION(instr, type) \
                case Instruction::instr: { \
                        type v2 = frame.PopValue<type>(); \
                        type v1 = frame.PopValue<type>(); \
                        if(v1 > v2) { \
                            frame.CreatePush<int>(-1); \
                        } \
                        else if(v1 < v2) { \
                            frame.CreatePush<int>(1); \
                        } \
                        else { \
                            frame.CreatePush<int>(0); \
                        } \
                        break; \
                    }

                switch(inst) {
                    case Instruction::NOP: {
                        // Do nothing :P
                        break;
                    }
                    case Instruction::ACONST_NULL: {
                        frame.Push(CreateNullValue());
                        break;
                    }
                    case Instruction::ICONST_M1: {
                        frame.CreatePush<int>(-1);
                        break;
                    }
                    case Instruction::ICONST_0: {
                        frame.CreatePush<int>(0);
                        break;
                    }
                    case Instruction::ICONST_1: {
                        frame.CreatePush<int>(1);
                        break;
                    }
                    case Instruction::ICONST_2: {
                        frame.CreatePush<int>(2);
                        break;
                    }
                    case Instruction::ICONST_3: {
                        frame.CreatePush<int>(3);
                        break;
                    }
                    case Instruction::ICONST_4: {
                        frame.CreatePush<int>(4);
                        break;
                    }
                    case Instruction::ICONST_5: {
                        frame.CreatePush<int>(5);
                        break;
                    }
                    case Instruction::LCONST_0: {
                        frame.CreatePush<long>(0);
                        break;
                    }
                    case Instruction::LCONST_1: {
                        frame.CreatePush<long>(1);
                        break;
                    }
                    case Instruction::FCONST_0: {
                        frame.CreatePush<float>(0.0f);
                        break;
                    }
                    case Instruction::FCONST_1: {
                        frame.CreatePush<float>(1.0f);
                        break;
                    }
                    case Instruction::FCONST_2: {
                        frame.CreatePush<float>(2.0f);
                        break;
                    }
                    case Instruction::DCONST_0: {
                        frame.CreatePush<double>(0.0f);
                        break;
                    }
                    case Instruction::DCONST_1: {
                        frame.CreatePush<double>(1.0f);
                        break;
                    }
                    case Instruction::BIPUSH: {
                        u8 byte = frame.Read<u8>();
                        frame.CreatePush<int>((int)byte);
                        break;
                    }
                    case Instruction::SIPUSH: {
                        i16 shrt = BE(frame.Read<i16>());
                        frame.CreatePush<int>((int)shrt);
                        break;
                    }
                    _JAVM_LDC_INSTRUCTION(LDC, frame.Read<u8>())
                    _JAVM_LDC_INSTRUCTION(LDC_W, BE(frame.Read<u16>()))
                    _JAVM_LDC_INSTRUCTION(LDC2_W, BE(frame.Read<u16>()))
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
                        frame.Pop();
                        break;
                    }
                    case Instruction::POP2: {
                        frame.Pop();
                        frame.Pop();
                        break;
                    }
                    case Instruction::DUP: {
                        auto val = frame.Pop();
                        frame.Push(val);
                        frame.Push(val);
                        break;
                    }
                    case Instruction::DUP_X1: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        frame.Push(v1);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP_X2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        frame.Push(v1);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2_X1: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    case Instruction::DUP2_X2: {
                        auto v1 = frame.Pop();
                        auto v2 = frame.Pop();
                        auto v3 = frame.Pop();
                        auto v4 = frame.Pop();
                        frame.Push(v2);
                        frame.Push(v1);
                        frame.Push(v4);
                        frame.Push(v3);
                        frame.Push(v2);
                        frame.Push(v1);
                        break;
                    }
                    _JAVM_OPERATOR_INSTRUCTION(IADD, int, +)
                    _JAVM_OPERATOR_INSTRUCTION(LADD, long, +)
                    _JAVM_OPERATOR_INSTRUCTION(FADD, float, +)
                    _JAVM_OPERATOR_INSTRUCTION(DADD, double, +)
                    _JAVM_OPERATOR_INSTRUCTION(ISUB, int, -)
                    _JAVM_OPERATOR_INSTRUCTION(LSUB, long, -)
                    _JAVM_OPERATOR_INSTRUCTION(FSUB, float, -)
                    _JAVM_OPERATOR_INSTRUCTION(DSUB, double, -)
                    _JAVM_OPERATOR_INSTRUCTION(IMUL, int, *)
                    _JAVM_OPERATOR_INSTRUCTION(LMUL, long, *)
                    _JAVM_OPERATOR_INSTRUCTION(FMUL, float, *)
                    _JAVM_OPERATOR_INSTRUCTION(DMUL, double, *)
                    _JAVM_OPERATOR_INSTRUCTION(IDIV, int, /)
                    _JAVM_OPERATOR_INSTRUCTION(LDIV, long, /)
                    _JAVM_OPERATOR_INSTRUCTION(FDIV, float, /)
                    _JAVM_OPERATOR_INSTRUCTION(DDIV, double, /)
                    _JAVM_OPERATOR_INSTRUCTION(IREM, int, %)
                    _JAVM_OPERATOR_INSTRUCTION(LREM, long, %)
                    // Fuck u, decimals
                    case Instruction::FREM: {
                        float v2 = frame.PopValue<float>();
                        float v1 = frame.PopValue<float>();
                        frame.CreatePush<float>(fmodf(v1, v2));
                        break;
                    }
                    case Instruction::DREM: {
                        double v2 = frame.PopValue<double>();
                        double v1 = frame.PopValue<double>();
                        frame.CreatePush<double>(fmod(v1, v2));
                        break;
                    }
                    _JAVM_NEG_INSTRUCTION(INEG, int)
                    _JAVM_NEG_INSTRUCTION(LNEG, long)
                    _JAVM_NEG_INSTRUCTION(FNEG, float)
                    _JAVM_NEG_INSTRUCTION(DNEG, double)
                    case Instruction::ISHL: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        frame.CreatePush<int>(v1 << tmp);
                        break;
                    }
                    case Instruction::LSHL: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        frame.CreatePush<long>(v1 << tmp);
                        break;
                    }
                    case Instruction::ISHR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        frame.CreatePush<int>(v1 >> tmp);
                        break;
                    }
                    case Instruction::LSHR: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        frame.CreatePush<long>(v1 >> tmp);
                        break;
                    }
                    case Instruction::IUSHR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        auto tmp = v2 & 0x1F;
                        if(v1 >= 0) {
                            frame.CreatePush<int>(v1 >> tmp);
                        }
                        else {
                            frame.CreatePush<int>((v1 >> tmp) + (2 << ~tmp));
                        }
                        break;
                    }
                    case Instruction::LUSHR: {
                        int v2 = frame.PopValue<int>();
                        long v1 = frame.PopValue<long>();
                        auto tmp = v2 & 0x3F;
                        if(v1 >= 0) {
                            frame.CreatePush<long>(v1 >> tmp);
                        }
                        else {
                            frame.CreatePush<long>((v1 >> tmp) + (2 << ~tmp));
                        }
                        break;
                    }
                    case Instruction::IAND: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 & v2);
                        break;
                    }
                    case Instruction::LAND: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 & v2);
                        break;
                    }
                    case Instruction::IOR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 | v2);
                        break;
                    }
                    case Instruction::LOR: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 | v2);
                        break;
                    }
                    case Instruction::IXOR: {
                        int v2 = frame.PopValue<int>();
                        int v1 = frame.PopValue<int>();
                        frame.CreatePush<int>(v1 ^ v2);
                        break;
                    }
                    case Instruction::LXOR: {
                        long v2 = frame.PopValue<long>();
                        long v1 = frame.PopValue<long>();
                        frame.CreatePush<long>(v1 ^ v2);
                        break;
                    }
                    case Instruction::IINC: {
                        u8 idx = frame.Read<u8>();
                        i8 cnst = frame.Read<i8>();
                        auto value = frame.GetLocalValue<int>((u32)idx);
                        value += cnst;
                        frame.CreateSetLocal<int>((u32)idx, value);
                        break;
                    }
                    _JAVM_CONVERSION_INSTRUCTION(I2L, int, long)
                    _JAVM_CONVERSION_INSTRUCTION(I2F, int, float)
                    _JAVM_CONVERSION_INSTRUCTION(I2D, int, double)
                    _JAVM_CONVERSION_INSTRUCTION(L2I, long, int)
                    _JAVM_CONVERSION_INSTRUCTION(L2F, long, float)
                    _JAVM_CONVERSION_INSTRUCTION(L2D, long, double)
                    _JAVM_CONVERSION_INSTRUCTION(F2I, float, int)
                    _JAVM_CONVERSION_INSTRUCTION(F2L, float, long)
                    _JAVM_CONVERSION_INSTRUCTION(F2D, float, double)
                    _JAVM_CONVERSION_INSTRUCTION(D2I, double, int)
                    _JAVM_CONVERSION_INSTRUCTION(D2L, double, long)
                    _JAVM_CONVERSION_INSTRUCTION(D2F, double, float)
                    _JAVM_CONVERSION_INSTRUCTION(I2B, int, u8)
                    _JAVM_CONVERSION_INSTRUCTION(I2C, int, char)
                    _JAVM_CONVERSION_INSTRUCTION(I2S, int, short)
                    _JAVM_CMP_INSTRUCTION(LCMP, long)
                    _JAVM_CMP_INSTRUCTION(FCMPL, float)
                    _JAVM_CMP_INSTRUCTION(FCMPG, float)
                    _JAVM_CMP_INSTRUCTION(DCMPL, double)
                    _JAVM_CMP_INSTRUCTION(DCMPG, double)

                    case Instruction::IFEQ: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v == 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFNE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v != 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFLT: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v < 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFGE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v >= 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFGT: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v > 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFLE: {
                        int v = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v <= 0) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPEQ: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 == v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPNE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 != v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPLT: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 < v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPGE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 >= v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPGT: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 > v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ICMPLE: {
                        auto v2 = frame.PopValue<int>();
                        auto v1 = frame.PopValue<int>();
                        i16 branch = BE(frame.Read<i16>());
                        if(v1 <= v2) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ACMPEQ: {
                        auto v2 = frame.Pop();
                        auto v1 = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if((v1->GetAddress() == v2->GetAddress()) && (v1->GetTypeHashCode() == v2->GetTypeHashCode()) && (v1->GetValueType() == v2->GetValueType())) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IF_ACMPNE: {
                        auto v2 = frame.Pop();
                        auto v1 = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if((v1->GetAddress() != v2->GetAddress()) || (v1->GetTypeHashCode() != v2->GetTypeHashCode()) || (v1->GetValueType() != v2->GetValueType())) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::GOTO: {
                        i16 branch = BE(frame.Read<i16>());
                        offset -= 3;
                        offset += branch;
                        break;
                    }
                    case Instruction::JSR:
                    case Instruction::RET: {
                        // Old instructions, not implementing them
                        break;
                    }
                    // TODO: TABLESWITCH, LOOKUPSWITCH
                    case Instruction::IRETURN: {
                        int ret = frame.PopValue<int>();
                        should_ret = true;
                        return CreateNewValue<int>(ret);
                    }
                    case Instruction::LRETURN: {
                        long ret = frame.PopValue<long>();
                        should_ret = true;
                        return CreateNewValue<long>(ret);
                    }
                    case Instruction::ARETURN: {
                        should_ret = true;
                        return frame.Pop();
                    }
                    case Instruction::FRETURN: {
                        float ret = frame.PopValue<float>();
                        should_ret = true;
                        return CreateNewValue<float>(ret);
                    }
                    case Instruction::DRETURN: {
                        double ret = frame.PopValue<double>();
                        should_ret = true;
                        return CreateNewValue<double>(ret);
                    }
                    case Instruction::RETURN: {
                        should_ret = true;
                        // Return nothing - a void value
                        return CreateVoidValue();    
                    }
                    case Instruction::GETSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            if(class_def->CanHandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame)) {
                                auto params = ClassObject::LoadStaticFunctionParameters(frame, JAVM_EMPTY_METHOD_DESCRIPTOR);
                                class_def->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, params, frame);
                            }
                            auto obj = class_def->GetStaticField(fld_nat_data.processed_name);
                            JAVM_ASSERT_VALID_VALUE(this, obj, "Invalid static field - " + fld_nat_data.processed_name, {
                                if(obj->IsVoid()) {
                                    this->ThrowRuntimeException("Invalid static field - " + fld_nat_data.processed_name);
                                }
                                else {
                                    frame.Push(obj);
                                }
                            })
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::PUTSTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            if(class_def->CanHandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, frame)) {
                                auto params = ClassObject::LoadStaticFunctionParameters(frame, JAVM_EMPTY_METHOD_DESCRIPTOR);
                                class_def->HandleStaticFunction(JAVM_STATIC_BLOCK_METHOD_NAME, JAVM_EMPTY_METHOD_DESCRIPTOR, params, frame);
                            }

                            auto obj = frame.Pop();
                            JAVM_ASSERT_VALID_VALUE(this, obj, "Invalid value to be assigned to static field " + fld_nat_data.processed_name, {
                                if(obj->IsVoid()) {
                                    this->ThrowRuntimeException("Invalid value to be assigned to static field " + fld_nat_data.processed_name);
                                }
                                else {
                                    class_def->SetStaticField(fld_nat_data.processed_name, obj);
                                }
                            })
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::GETFIELD: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        auto value = frame.Pop();
                        if(value->IsClassObject()) {
                            auto clss = value->GetReference<ClassObject>();
                            if(clss->HasField(fld_nat_data.processed_name)) {
                                auto obj = clss->GetField(fld_nat_data.processed_name);
                                JAVM_ASSERT_VALID_VALUE(this, obj, "Invalid value of field " + fld_nat_data.processed_name, {
                                    if(obj->IsVoid()) {
                                        this->ThrowRuntimeException("Invalid value of field " + fld_nat_data.processed_name);
                                    }
                                    else {
                                        frame.Push(obj);
                                    }
                                })
                            }
                            else {
                                this->ThrowRuntimeException("Unable to find field " + fld_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowRuntimeException("Invalid variable, expected an object");
                        }
                        break;
                    }
                    case Instruction::PUTFIELD: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fld_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();
                        
                        auto value = frame.Pop();
                        JAVM_ASSERT_VALID_VALUE(this, value, "Invalid value to be assigned to field " + fld_nat_data.processed_name, {
                            auto cls_value = frame.Pop();
                            if(cls_value->IsClassObject()) {
                                auto clss = cls_value->GetReference<ClassObject>();
                                clss->SetField(fld_nat_data.processed_name, value);
                            }
                            else {
                                this->ThrowRuntimeException("Invalid variable, expected an object");
                            }
                        })
                        break;
                    }
                    case Instruction::INVOKEVIRTUAL: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        auto [this_v, params] = ClassObject::LoadMethodParameters(frame, fn_nat_data.processed_desc);
                        if(!this_v->IsClassObject()) {
                            this->ThrowRuntimeException("Invalid input");
                            break;
                        }

                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            if(class_def->CanAllHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                bool valid_names = false;
                                if(this_v->IsClassObject()) {
                                    auto this_class_obj = this_v->GetReference<ClassObject>();
                                    auto this_class_name = this_class_obj->GetName();

                                    // Virtual invoke - only own class or superclass methods
                                    if(this_class_name == class_name) {
                                        valid_names = true;
                                    }
                                    auto super_class_v = this_class_obj->GetSuperClassInstance();
                                    while(super_class_v) {
                                        if(valid_names) {
                                            break;
                                        }
                                        auto super_class_obj = super_class_v->GetReference<ClassObject>();
                                        auto super_class_name = super_class_obj->GetName();
                                        if(super_class_name == class_name) {
                                            valid_names = true;
                                            break;
                                        }
                                        super_class_v = super_class_obj->GetSuperClassInstance();
                                    }
                                }
                                if(!valid_names) {
                                    this->ThrowRuntimeException("Invalid input parameters of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                                }
                                // Once we do know the class object we were sent can handle the method, execute it
                                auto this_class_obj = this_v->GetReference<ClassObject>();
                                auto ret = this_class_obj->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, this_v, params, frame);
                                JAVM_ASSERT_VALID_VALUE(this, ret, "Invalid return value of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name, {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    if(should_ret) {
                                        if(ret->IsVoid()) {
                                            this->ThrowRuntimeException("Invalid return value of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                                        }
                                        else {
                                            frame.Push(ret);
                                        }
                                    }
                                })
                            }
                            else {
                                this->ThrowRuntimeException("Unable to find or call method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::INVOKEINTERFACE: {
                        u16 index = BE(frame.Read<u16>());
                        u8 count = frame.Read<u8>(); // Both unused...?
                        u8 zero = frame.Read<u8>();
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        auto [this_v, params] = ClassObject::LoadMethodParameters(frame, fn_nat_data.processed_desc);
                        if(!this_v->IsClassObject()) {
                            this->ThrowRuntimeException("Invalid input");
                            break;
                        }
                        

                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            if(class_def->CanAllHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                bool valid_names = false;
                                if(this_v->IsClassObject()) {
                                    auto this_class_obj = this_v->GetReference<ClassObject>();
                                    auto this_class_name = this_class_obj->GetName();

                                    // Interface - thus it could be a interface of the class or subclasses
                                    if(this_class_name == class_name) {
                                        valid_names = true;
                                    }
                                    for(auto intf: this_class_obj->GetInterfaceNames()) {
                                        if(intf == class_name) {
                                            valid_names = true;
                                            break;
                                        }
                                    }
                                    auto super_class_v = this_class_obj->GetSuperClassInstance();
                                    while(super_class_v) {
                                        if(valid_names) {
                                            break;
                                        }
                                        auto super_class_obj = super_class_v->GetReference<ClassObject>();
                                        auto super_class_name = super_class_obj->GetName();
                                        if(super_class_name == class_name) {
                                            valid_names = true;
                                            break;
                                        }
                                        for(auto intf: super_class_obj->GetInterfaceNames()) {
                                            if(intf == class_name) {
                                                valid_names = true;
                                                break;
                                            }
                                        }
                                        super_class_v = super_class_obj->GetSuperClassInstance();
                                    }
                                }
                                if(!valid_names) {
                                    this->ThrowRuntimeException("Invalid input parameters of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                                }
                                // Once we do know the class object we were sent can handle the method, execute it
                                auto this_class_obj = this_v->GetReference<ClassObject>();
                                auto ret = this_class_obj->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, this_v, params, frame);
                                JAVM_ASSERT_VALID_VALUE(this, ret, "Invalid return value of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name, {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    if(should_ret) {
                                        if(ret->IsVoid()) {
                                            this->ThrowRuntimeException("Invalid return value of method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                                        }
                                        else {
                                            frame.Push(ret);
                                        }
                                    }
                                })
                            }
                            else {
                                this->ThrowRuntimeException("Unable to find or call method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::INVOKESPECIAL: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        auto [this_v, params] = ClassObject::LoadMethodParameters(frame, fn_nat_data.processed_desc);
                        if(!this_v->IsClassObject()) {
                            this->ThrowRuntimeException("Invalid input");
                            break;
                        }

                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            if(class_def->CanAllHandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                class_def->HandleMethod(fn_nat_data.processed_name, fn_nat_data.processed_desc, this_v, params, frame);
                            }
                            else {
                                this->ThrowRuntimeException("Unable to find or call special method " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::INVOKESTATIC: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto fn_data = constant_pool[index - 1].GetFieldMethodRefData();
                        auto class_name = constant_pool[fn_data.class_index - 1].GetClassData().processed_name;
                        auto fn_nat_data = constant_pool[fn_data.name_and_type_index - 1].GetNameAndTypeData();

                        auto params = ClassObject::LoadStaticFunctionParameters(frame, fn_nat_data.processed_desc);

                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                            if(class_def->CanAllHandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, frame)) {
                                auto ret = class_def->HandleStaticFunction(fn_nat_data.processed_name, fn_nat_data.processed_desc, params, frame);
                                JAVM_ASSERT_VALID_VALUE(this, ret, "Invalid return value of static function " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name, {
                                    bool should_ret = ClassObject::ExpectsReturn(fn_nat_data.processed_desc);
                                    if(should_ret) {
                                        if(ret->IsVoid()) {
                                            this->ThrowRuntimeException("Invalid (void) return value of static function " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                                        }
                                        else {
                                            frame.Push(ret);
                                        }
                                    }
                                })
                            }
                            else {
                                this->ThrowRuntimeException("Unable to find or call static function " + ClassObject::GetPresentableClassName(class_name) + "." + fn_nat_data.processed_name);
                            }
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    // TODO: INVOKEDYNAMIC
                    case Instruction::NEW: {
                        u16 index = BE(frame.Read<u16>());
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;

                        if(this->HasClass(class_name)) {
                            auto class_def = this->FindClass(class_name);
                            auto obj = class_def->CreateInstance(frame);
                            frame.Push(obj);
                        }
                        else {
                            this->ThrowClassNotFound(class_name);
                        }

                        break;
                    }
                    case Instruction::NEWARRAY: {
                        u8 type = frame.Read<u8>();
                        auto vtype = this->GetValueTypeFromNewArrayType(static_cast<NewArrayType>(type));
                        if(vtype == ValueType::Void) {
                            this->ThrowRuntimeException("Invalid array primitive type");
                        }
                        else {
                            int len = frame.PopValue<int>();
                            if(len < 0) {
                                this->ThrowRuntimeException("Array size cannot be negative");
                            }
                            else {
                                auto arr_value = CreateNewValue<Array>(vtype, len);
                                frame.Push(arr_value);
                            }
                        }
                        break;
                    }
                    case Instruction::ANEWARRAY: {
                        u16 idx = BE(frame.Read<u16>());
                        int len = frame.PopValue<int>();
                        if(len < 0) {
                            this->ThrowRuntimeException("Array size cannot be negative");
                        }
                        else {
                            auto arr_value = CreateNewValue<Array>(ValueType::ClassObject, len);
                            frame.Push(arr_value);
                        }
                        break;
                    }
                    case Instruction::ARRAYLENGTH: {
                        auto value = frame.Pop();
                        if(value->IsArray()) {
                            auto arr = value->GetReference<Array>();
                            frame.CreatePush<int>((int)arr->GetLength());
                        }
                        else {
                            this->ThrowRuntimeException("Invalid input variable (not an array)");
                        }
                        break;
                    }
                    case Instruction::ATHROW: {
                        this->ThrowWithInstance(frame, frame.Pop());
                        should_ret = true;
                        break;
                    }
                    case Instruction::CHECKCAST: {
                        u16 index = BE(frame.Read<u16>());
                        auto value = frame.Pop();
                        auto obj = value->GetReference<ClassObject>();
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;
                        if(obj->CanCastTo(class_name)) {
                            frame.Push(value);
                        }
                        else {
                            this->ThrowRuntimeException(ClassObject::GetPresentableClassName(obj->GetName()) + " cannot be cast to "+  ClassObject::GetPresentableClassName(class_name));
                        }
                        break;
                    }
                    case Instruction::INSTANCEOF: {
                        u16 index = BE(frame.Read<u16>());
                        auto value = frame.Pop();
                        auto obj = value->GetReference<ClassObject>();
                        auto &constant_pool = frame.GetCurrentClass()->GetConstantPool();
                        auto class_name = constant_pool[index - 1].GetClassData().processed_name;
                        if(obj->CanCastTo(class_name)) {
                            frame.CreatePush<int>(1);
                        }
                        else {
                            frame.CreatePush<int>(0);
                        }
                        break;
                    }
                    case Instruction::IFNULL: {
                        auto value = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if(value->IsNull()) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::IFNONNULL: {
                        auto value = frame.Pop();
                        i16 branch = BE(frame.Read<i16>());
                        if(!value->IsNull()) {
                            offset -= 3;
                            offset += branch;
                        }
                        break;
                    }
                    case Instruction::GOTO_W:
                    case Instruction::JSR_W: {
                        // Old instructions, not implementing them
                        break;
                    }
                    
                    default:
                        this->ThrowRuntimeException("Unknown or unimplemented opcode/instruction: 0x" + ToHexString(static_cast<u32>(inst)));
                        break;
                }

                return CreateInvalidValue();
            }

        public:
            Machine() {
                this->Reset();
            }

            void Reset() {
                this->loaded_archives.clear();
                this->class_files.clear();
                this->native_classes.clear();
                this->native_function_classes.clear();
                this->thrown_info = {};
                this->thrown_info.info_valid = false;
            }

            ThrownInfo GetExceptionInfo() {
                return this->thrown_info;
            }

            bool WasExceptionThrown() {
                return this->thrown_info.info_valid;
            }

            void ThrowWithInstance(Frame &frame, Value value) {
                if(this->WasExceptionThrown()) {
                    // Another exception is already thrown
                    return;
                }
                auto obj = value->GetReference<ClassObject>();
                auto str = obj->CallMethod(frame, "getMessage", TypeDefinitions::GetClassDefinition(reinterpret_cast<void*>(this), "java.lang.String")); // Throwable will be a super class, so calls Throwable.getMessage()
                JAVM_ASSERT_VALID_VALUE(this, str, "Invalid exception message type", {
                    if(str->IsVoid()) {
                        this->ThrowRuntimeException("Invalid exception message type");
                    }
                    else {
                        this->thrown_info = {};
                        this->thrown_info.info_valid = true;
                        this->thrown_info.class_type = ClassObject::GetPresentableClassName(obj->GetName());
                        this->thrown_info.message = "<no message>";
                        if(str->IsValidCast<java::lang::String>()) {
                            auto str_ref = str->GetReference<java::lang::String>();
                            this->thrown_info.message = str_ref->GetNativeString();
                        }
                        else {
                            // We just don't care if the message isn't a String, just ignore it and throw without a custom message :P
                        }
                    }
                })
            }

            void ThrowWithType(const std::string &class_name, const std::string &message) {
                if(this->WasExceptionThrown()) {
                    // Another exception is already thrown
                    return;
                }
                if(this->HasClass(class_name)) {

                    /* Java pseudocode:
                        String str = message;
                        <class_name> t = new <class_name>(str);
                        <throw exception internally>(t);
                    */

                    auto str_obj = CreateNewClassWith<true>(this, "java.lang.String", [&](auto *ref) {
                        reinterpret_cast<java::lang::String*>(ref)->SetNativeString(message);
                    });

                    auto throwable_val = CreateNewClass<true>(this, class_name, str_obj);
                    Frame frame(reinterpret_cast<void*>(this));
                    this->ThrowWithInstance(frame, throwable_val);
                }
                else {
                    this->ThrowClassNotFound(class_name);
                }
            }

            void ThrowWithMessage(const std::string &message) {
                this->ThrowWithType("java.lang.Exception", message);
            }

            // Simplify getting the machine pointer from a frame
            static Machine *GetFrameMachinePointer(Frame &frame) {
                return reinterpret_cast<Machine*>(frame.GetMachinePointer());
            }

            template<typename ...Args>
            std::shared_ptr<ClassFile> LoadClassFile(Args &&...args) {
                std::shared_ptr<ClassObject> classptr = ClassFile::CreateDefinitionInstance(reinterpret_cast<void*>(this), args...);
                this->class_files.push_back(std::move(classptr));
                return std::dynamic_pointer_cast<ClassFile>(this->class_files.back());
            }

            template<typename C, typename ...Args>
            void LoadNativeClass(Args &&...args) {
                static_assert(std::is_base_of_v<java::native::Class, C>, "Native classes must inherit from from java::native::Class!");

                std::shared_ptr<native::Class> classptr = C::CreateDefinitionInstance(reinterpret_cast<void*>(this));
                this->native_classes.push_back(std::move(classptr));
            }

            template<typename C, typename ...Args>
            void LoadNativeFunctionClass(Args &&...args) {
                static_assert(std::is_base_of_v<java::native::Class, C>, "Native function classes must inherit from from java::native::Class!");

                std::shared_ptr<native::Class> classptr = C::CreateDefinitionInstance(reinterpret_cast<void*>(this));
                this->native_function_classes.push_back(std::move(classptr));
            }

            void LoadBuiltinNativeClasses() {
                this->LoadNativeClass<java::lang::Object>();
                this->LoadNativeClass<java::lang::String>();
                this->LoadNativeClass<java::lang::StringBuilder>();
                this->LoadNativeClass<java::lang::Class>();
                this->LoadNativeClass<java::lang::Enum>();
                this->LoadNativeClass<java::io::PrintStream>();
                this->LoadNativeClass<java::lang::System>();
                this->LoadNativeClass<java::lang::Throwable>();
                this->LoadNativeClass<java::lang::Exception>();
                this->LoadNativeClass<java::lang::RuntimeException>();
                this->LoadNativeClass<java::lang::IllegalArgumentException>();
                this->LoadNativeClass<java::lang::CloneNotSupportedException>();
                this->LoadNativeClass<java::lang::Error>();
                this->LoadNativeClass<java::lang::LinkageError>();
                this->LoadNativeClass<java::lang::NoClassDefFoundError>();
            }

            template<typename ...Args>
            std::shared_ptr<Archive> &LoadJavaArchive(Args &&...args) {
                auto archiveptr = std::make_shared<Archive>(reinterpret_cast<void*>(this), args...);
                this->loaded_archives.push_back(std::move(archiveptr));
                return this->loaded_archives.back();
            }

            bool HasClass(const std::string &name) {
                auto cls_name = ClassObject::ProcessClassName(name);
                for(auto &native: this->native_classes) {
                    if(native->GetName() == cls_name) {
                        return true;
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto &class_file: archive->GetLoadedClasses()) {
                        if(class_file->GetName() == cls_name) {
                            return true;
                        }
                    }
                }
                // Finally, plain .class files
                for(auto &class_file: this->class_files) {
                    if(class_file->GetName() == cls_name) {
                        return true;
                    }
                }
                return false;
            }
            
            std::shared_ptr<ClassObject> FindClass(const std::string &name) {
                auto cls_name = ClassObject::ProcessClassName(name);
                for(auto &native: this->native_classes) {
                    if(native->GetName() == cls_name) {
                        return native;
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto &class_file: archive->GetLoadedClasses()) {
                        if(class_file->GetName() == cls_name) {
                            return class_file;
                        }
                    }
                }
                // Finally, plain .class files
                for(auto &class_file: this->class_files) {
                    if(class_file->GetName() == cls_name) {
                        return class_file;
                    }
                }
                return this->empty_class;
            }

            bool HasNativeMethod(const std::string &class_name, const std::string &native_fn_name) {
                auto cls_name = ClassObject::ProcessClassName(class_name);
                for(auto &native: this->native_function_classes) {
                    if(native->GetName() == cls_name) {
                        Frame frame(reinterpret_cast<void*>(this));
                        if(native->CanHandleMethod(native_fn_name, "", frame)) {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool HasNativeStaticFunction(const std::string &class_name, const std::string &native_fn_name) {
                auto cls_name = ClassObject::ProcessClassName(class_name);
                for(auto &native: this->native_function_classes) {
                    if(native->GetName() == cls_name) {
                        Frame frame(reinterpret_cast<void*>(this));
                        if(native->CanHandleStaticFunction(native_fn_name, "", frame)) {
                            return true;
                        }
                    }
                }
                return false;
            }

            std::shared_ptr<ClassObject> FindNativeMethodClass(const std::string &class_name, const std::string &native_fn_name) {
                auto cls_name = ClassObject::ProcessClassName(class_name);
                for(auto &native: this->native_function_classes) {
                    if(native->GetName() == cls_name) {
                        Frame frame(reinterpret_cast<void*>(this));
                        if(native->CanHandleMethod(native_fn_name, "", frame)) {
                            return native;
                        }
                    }
                }
                return this->empty_class;
            }

            std::shared_ptr<ClassObject> FindNativeStaticFunctionClass(const std::string &class_name, const std::string &native_fn_name) {
                auto cls_name = ClassObject::ProcessClassName(class_name);
                for(auto &native: this->native_function_classes) {
                    if(native->GetName() == cls_name) {
                        Frame frame(reinterpret_cast<void*>(this));
                        if(native->CanHandleStaticFunction(native_fn_name, "", frame)) {
                            return native;
                        }
                    }
                }
                return this->empty_class;
            }

            Value ExecuteCode(Frame &frame) {
                while(true) {
                    if(this->WasExceptionThrown()) {
                        return CreateInvalidValue();
                    }

                    auto inst = static_cast<Instruction>(frame.Read<u8>());

                    if(frame.StackMaximum()) {
                        // WTF
                        // printf("Max stack!\n");
                        break;
                    }

                    bool should_ret = false;
                    auto ret = this->HandleInstruction(frame, should_ret, inst);
                    if(should_ret) {
                        return ret;
                    }
                }
                return CreateInvalidValue();
            }

            template<typename ...Args>
            Value CallFunction(const std::string &class_name, const std::string &fn_name, Args &&...args) {
                if(this->WasExceptionThrown()) {
                    return CreateInvalidValue();
                }
                
                auto proper_class_name = ClassObject::ProcessClassName(class_name);
                // Native classes have preference
                for(auto &native: this->native_classes) {
                    if(native->GetName() == proper_class_name) {
                        Frame frame(native->CreateFromExistingInstance(), reinterpret_cast<void*>(this));
                        ClassObject::PushParameters(frame, args...);
                        auto desc = ClassObject::BuildFunctionDescriptor(args...);
                        auto params = ClassObject::LoadStaticFunctionParameters(frame, desc);
                        return native->HandleStaticFunction(fn_name, desc, params, frame);
                    }
                }
                // Then, loaded JARs
                for(auto &archive: this->loaded_archives) {
                    for(auto class_def: archive->GetLoadedClasses()) {
                        auto class_file = std::dynamic_pointer_cast<ClassFile>(class_def);
                        if(class_file->GetName() == proper_class_name) {
                            for(auto &method: class_file->GetMethods()) {
                                if(method.GetName() == fn_name) {
                                    for(auto &attr: method.GetAttributes()) {
                                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                                            CodeAttribute code_attr(code_reader);
                                            Frame frame(&code_attr, class_file->CreateFromExistingInstance(), reinterpret_cast<void*>(this));
                                            u32 index = 0;
                                            ((this->HandlePushArgument(frame, index, args)), ...);
                                            auto ret = this->ExecuteCode(frame);
                                            code_attr.Dispose();
                                            return ret;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Finally, plain .class files
                for(auto class_def: this->class_files) {
                    auto class_file = std::dynamic_pointer_cast<ClassFile>(class_def);
                    if(class_file->GetName() == proper_class_name) {
                        for(auto &method: class_file->GetMethods()) {
                            if(method.GetName() == fn_name) {
                                for(auto &attr: method.GetAttributes()) {
                                    if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                                        MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                                        CodeAttribute code_attr(code_reader);
                                        Frame frame(&code_attr, class_file->CreateFromExistingInstance(), reinterpret_cast<void*>(this));
                                        u32 index = 0;
                                        ((this->HandlePushArgument(frame, index, args)), ...);
                                        auto ret = this->ExecuteCode(frame);
                                        code_attr.Dispose();
                                        return ret;
                                    }
                                }
                            }
                        }
                    }
                }
                return CreateInvalidValue();
            }
    };

    Value HandleClassFileMethod(ClassFile *class_file, const std::string &name, const std::string &desc, Value this_v, std::vector<Value> params, Frame &frame) {
        for(auto &method: class_file->GetMethods()) {
            if(method.GetName() == name) {
                if(method.GetDesc() == desc) {
                    for(auto &attr: method.GetAttributes()) {
                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                            CodeAttribute code_attr(code_reader);
                            Frame new_frame(&code_attr, class_file->CreateFromExistingInstance(), frame.GetMachinePointer());
                            new_frame.SetLocal(0, this_v);
                            for(u32 i = 0; i < params.size(); i++) {
                                new_frame.SetLocal(i + 1, params[i]);
                            }
                            auto ret = Machine::GetFrameMachinePointer(frame)->ExecuteCode(new_frame);
                            code_attr.Dispose();
                            return ret;
                        }
                    }
                    if(method.Is<AccessFlags::Native>()) {
                        auto class_name = class_file->GetName();
                        // Try to locate and call the native method
                        auto machine_ptr = Machine::GetFrameMachinePointer(frame);
                        if(machine_ptr->HasNativeMethod(class_name, name)) {
                            auto class_def = machine_ptr->FindNativeMethodClass(class_name, name);
                            return class_def->HandleMethod(name, desc, this_v, params, frame);
                        }
                    }
                }
            }
        }
        auto super_class = class_file->GetSuperClassInstance();
        if(super_class) {
            if(super_class->IsClassObject()) {
                auto super_class_ref = super_class->GetReference<ClassObject>();
                return super_class_ref->HandleMethod(name, desc, this_v, params, frame);
            }
        }
        return CreateInvalidValue();
    }

    Value HandleClassFileStaticFunction(ClassFile *class_file, const std::string &name, const std::string &desc, std::vector<Value> params, Frame &frame) {
        for(auto &method: class_file->GetMethods()) {
            if(method.GetName() == name) {
                if(method.GetDesc() == desc) {
                    for(auto &attr: method.GetAttributes()) {
                        if(attr.GetName() == JAVM_CODE_ATTRIBUTE_NAME) {
                            MemoryReader code_reader(attr.GetInfo(), attr.GetInfoLength());
                            CodeAttribute code_attr(code_reader);
                            Frame new_frame(&code_attr, class_file->CreateFromExistingInstance(), frame.GetMachinePointer());
                            for(u32 i = 0; i < params.size(); i++) {
                                new_frame.SetLocal(i, params[i]);
                            }
                            auto ret = Machine::GetFrameMachinePointer(frame)->ExecuteCode(new_frame);
                            code_attr.Dispose();
                            return ret;
                        }
                    }
                    if(method.Is<AccessFlags::Native>()) {
                        auto class_name = class_file->GetName();
                        // Try to locate and call the native method
                        auto machine_ptr = Machine::GetFrameMachinePointer(frame);
                        if(machine_ptr->HasNativeStaticFunction(class_name, name)) {
                            auto class_def = machine_ptr->FindNativeStaticFunctionClass(class_name, name);
                            return class_def->HandleStaticFunction(name, desc, params, frame);
                        }
                    }
                }
            }
        }
        auto super_class = class_file->GetSuperClassInstance();
        if(super_class) {
            if(super_class->IsClassObject()) {
                auto super_class_ref = super_class->GetReference<ClassObject>();
                return super_class_ref->HandleStaticFunction(name, desc, params, frame);
            }
        }
        return CreateInvalidValue();
    }

    std::shared_ptr<ClassObject> FindClassByNameEx(void *machine, const std::string &name) {
        auto mach = reinterpret_cast<Machine*>(machine);
        return mach->FindClass(name);
    }

    std::shared_ptr<ClassObject> FindClassByName(Frame &frame, const std::string &name) {
        return FindClassByNameEx(frame.GetMachinePointer(), name);
    }

    void MachineThrowWithMessage(void *machine, const std::string &message) {
        return reinterpret_cast<Machine*>(machine)->ThrowWithMessage(message);
    }

    void MachineThrowWithType(void *machine, const std::string &class_name, const std::string &message) {
        return reinterpret_cast<Machine*>(machine)->ThrowWithType(class_name, message);
    }

    void MachineThrowWithInstance(void *machine, Value value) {
        Frame frame(machine);
        return reinterpret_cast<Machine*>(machine)->ThrowWithInstance(frame, value);
    }

    // Class-creating helpers

    template<typename ...Args>
    inline void CallClassCtor(Frame &frame, Value class_val, Args &&...args) {
        auto class_ref = class_val->GetReference<ClassObject>();
        class_ref->CallMethod(frame, JAVM_CTOR_METHOD_NAME, CreateVoidValue(), args...);
    }

    template<bool CallCtor, typename ...Args>
    Value MachineCreateNewClass(void *machine, const std::string &name, Args &&...args) {
        auto machineptr = reinterpret_cast<Machine*>(machine);
        if(machineptr->HasClass(name)) {
            auto class_ref = machineptr->FindClass(name);
            Frame frame(machine);
            auto class_val = class_ref->CreateInstance(frame);
            if(CallCtor) {
                CallClassCtor(frame, class_val, args...);
            }
            return class_val;
        }
        return CreateInvalidValue();
    }

    template<bool CallCtor, typename ...Args>
    Value CreateNewClass(Machine &machine, const std::string &name, Args &&...args) {
        return MachineCreateNewClass<CallCtor>(&machine, name, args...);
    }

    template<bool CallCtor, typename ...Args>
    Value CreateNewClassWith(Machine &machine, const std::string &name, std::function<void(ClassObject*)> ref_fn, Args &&...args) {
        auto class_val = CreateNewClass<CallCtor>(machine, name, args...);
        auto class_ref = class_val->template GetReference<ClassObject>();
        ref_fn(class_ref);
        return class_val;
    }
}