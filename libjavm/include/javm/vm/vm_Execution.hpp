
#pragma once
#include <javm/vm/vm_Instructions.hpp>
#include <javm/vm/vm_Variable.hpp>

namespace javm::vm {

    class ExecutionFrame {

        private:
            enum class VariableStackMode {
                FixedSize, // Locals
                Growable, // Stack
            };

            class VariableStack {

                private:
                    std::vector<Ptr<Variable>> inner_list;
                    VariableStackMode stack_mode;

                public:
                    VariableStack(VariableStackMode mode) : stack_mode(mode) {}

                    void ResetFill(Ptr<Variable> this_var, u16 length) {
                        if(this->stack_mode != VariableStackMode::FixedSize) {
                            return;
                        }
                        if(this_var) {
                            length++;
                        }
                        this->inner_list.clear();
                        this->inner_list.reserve(length);
                        for(u16 i = 0; i < length; i++) {
                            this->inner_list.push_back(nullptr);
                        }
                    }

                    Ptr<Variable> GetAt(u32 idx) {
                        if(this->stack_mode != VariableStackMode::FixedSize) {
                            return nullptr;
                        }
                        if(idx < this->inner_list.size()) {
                            return this->inner_list[idx];
                        }
                        return nullptr;
                    }

                    void SetAt(u32 idx, Ptr<Variable> var) {
                        if(this->stack_mode != VariableStackMode::FixedSize) {
                            return;
                        }
                        if(idx < this->inner_list.size()) {
                            this->inner_list[idx] = var;
                        }
                    }

                    Ptr<Variable> Pop() {
                        if(this->stack_mode != VariableStackMode::Growable) {
                            return nullptr;
                        }
                        if(this->inner_list.empty()) {
                            return nullptr;
                        }
                        auto back_var = this->inner_list.back();
                        this->inner_list.pop_back();
                        return back_var;
                    }

                    void Push(Ptr<Variable> var) {
                        if(this->stack_mode != VariableStackMode::Growable) {
                            return;
                        }
                        this->inner_list.push_back(var);
                    }

            };

        private:
            VariableStack stack;
            VariableStack locals;
            ConstantPool exec_pool;
            u8 *code_ptr;
            u32 code_offset;

        public:
            ExecutionFrame(u8 *raw_code, u16 max_locals, ConstantPool pool, Ptr<Variable> this_var = nullptr) : stack(VariableStackMode::Growable), locals(VariableStackMode::FixedSize), exec_pool(pool), code_ptr(raw_code), code_offset(0) {
                this->locals.ResetFill(this_var, max_locals);
            }

            ConstantPool &GetThisConstantPool() {
                return this->exec_pool;
            }

            Ptr<Variable> GetLocalAt(u32 idx) {
                auto v = this->locals.GetAt(idx);
                if(!v) {
                    JAVM_LOG("Getting empty local var at [%d]...", idx);
                }
                return v;
            }

            void SetLocalAt(u32 idx, Ptr<Variable> var) {
                if(!var) {
                    JAVM_LOG("Setting empty local var at [%d]...", idx);
                }
                else {
                    JAVM_LOG("Setting local var at [%d]...", idx);
                }
                this->locals.SetAt(idx, var);
            }

            Ptr<Variable> PopStack() {
                return this->stack.Pop();
            }

            void PushStack(Ptr<Variable> var) {
                if(!var) {
                    JAVM_LOG("Pushing empty var into stack...");
                }
                this->stack.Push(var);
            }

            template<typename T>
            T ReadCode(bool forward = true) {
                T t = T();
                if(this->code_ptr == nullptr) {
                    return t;
                }

                memcpy(&t, (T*)&this->code_ptr[this->code_offset], sizeof(T));
                if(forward) {
                    this->code_offset += sizeof(T);
                }
                return t;
            }

            u32 &GetCodePosition() {
                return this->code_offset;
            }
    };

}