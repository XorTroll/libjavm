
#pragma once
#include <javm/vm/vm_Instructions.hpp>
#include <javm/vm/vm_Variable.hpp>
#include <javm/vm/jutil/jutil_Throwable.hpp>

namespace javm::vm {

    class ExecutionFrame {
        private:
            enum class VariableStackMode {
                FixedSize, // Locals
                Growable // Stack
            };

            class VariableStack {
                private:
                    std::vector<Ptr<Variable>> inner_list;
                    VariableStackMode stack_mode;

                    constexpr inline bool IsFixedSize() {
                        return this->stack_mode == VariableStackMode::FixedSize;
                    }

                    constexpr inline bool IsGrowable() {
                        return this->stack_mode == VariableStackMode::Growable;
                    }

                public:
                    VariableStack(const VariableStackMode mode) : stack_mode(mode) {}

                    void ResetFill(Ptr<Variable> this_var, const u16 length);
                    Ptr<Variable> GetAt(const u32 idx);
                    void SetAt(const u32 idx, Ptr<Variable> var);

                    Ptr<Variable> Pop();
                    void Push(Ptr<Variable> var);
            };

        private:
            VariableStack stack;
            VariableStack locals;
            ConstantPool &exec_pool;
            std::vector<ExceptionTableEntry> exc_table;
            const u8 *code_ptr;
            u32 code_offset;

        public:
            ExecutionFrame(const u8 *raw_code, const u16 max_locals, const std::vector<ExceptionTableEntry> &exc_table, ConstantPool &pool, Ptr<Variable> this_var = nullptr) : stack(VariableStackMode::Growable), locals(VariableStackMode::FixedSize), exec_pool(pool), exc_table(exc_table), code_ptr(raw_code), code_offset(0) {
                this->locals.ResetFill(this_var, max_locals);
            }

            inline ConstantPool &GetThisConstantPool() {
                return this->exec_pool;
            }

            inline Ptr<Variable> GetLocalAt(const u32 idx) {
                return this->locals.GetAt(idx);
            }

            inline void SetLocalAt(const u32 idx, Ptr<Variable> var) {
                this->locals.SetAt(idx, var);
            }

            inline Ptr<Variable> PopStack() {
                return this->stack.Pop();
            }

            inline void PushStack(Ptr<Variable> var) {
                this->stack.Push(var);
            }

            template<typename T>
            inline T ReadCode() {
                auto t = T();
                if(this->code_ptr == nullptr) {
                    return t;
                }

                memcpy(&t, reinterpret_cast<const T*>(&this->code_ptr[this->code_offset]), sizeof(T));
                this->code_offset += sizeof(T);
                return t;
            }

            inline u32 &GetCodeOffset() {
                return this->code_offset;
            }

            std::vector<ExceptionTableEntry> GetAvailableExceptionTableEntries(const u32 base_code_offset);
    };

    class ExecutionScopeGuard {
        private:
            bool self_thrown;

        public:
            ExecutionScopeGuard(Ptr<ClassType> type, const String &name, const String &descriptor);
            ~ExecutionScopeGuard();

            void NotifyThrown();
    };

    ExecutionResult ExecuteStaticCode(const u8 *code_ptr, const u16 max_locals, const std::vector<ExceptionTableEntry> &exc_table, ConstantPool &pool, const std::vector<Ptr<Variable>> &param_vars);
    ExecutionResult ExecuteCode(const u8 *code_ptr, const u16 max_locals, const std::vector<ExceptionTableEntry> &exc_table, Ptr<Variable> this_var, ConstantPool &pool, const std::vector<Ptr<Variable>> &param_vars);

    inline ExecutionResult ThrowExisting(Ptr<Variable> throwable_v, const bool is_catchable = true) {
        return ExecutionResult::Throw(throwable_v, is_catchable);
    }

    inline ExecutionResult Throw(const String &type, const String &msg = u"", const bool is_catchable = true) {
        return ThrowExisting(jutil::NewThrowable(type, msg), is_catchable);
    }

    inline ExecutionResult ThrowInternal(const String &msg) {
        JAVM_LOG("INTERNAL THROW: '%s'", str::ToUtf8(msg).c_str());
        return ThrowExisting(jutil::NewInternalThrowable(msg), false);
    }

}