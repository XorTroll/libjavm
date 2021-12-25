
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl::java::lang {

    using namespace vm;

    class Throwable {
        public:
            static ExecutionResult fillInStackTrace(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.lang.Throwable.fillInStackTrace] called");
                return ExecutionResult::ReturnVariable(this_var);
            }

            static ExecutionResult getStackTraceDepth(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                const auto cur_stack = ThreadUtils::GetCurrentCallStack();
                JAVM_LOG("[java.lang.Throwable.getStackTraceDepth] called - stack size: %ld", cur_stack.size());
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(cur_stack.size()));
            }

            static ExecutionResult getStackTraceElement(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                const auto cur_stack = ThreadUtils::GetCurrentCallStack();

                auto idx_v = param_vars[0];
                const auto idx = idx_v->GetValue<type::Integer>();

                const auto &call_info = cur_stack.at(idx);
                auto stack_trace_elem_v = TypeUtils::NewClassVariable(rt::LocateClassType(u"java/lang/StackTraceElement"));
                auto stack_trace_elem_obj = stack_trace_elem_v->GetAs<type::ClassInstance>();

                // TODO: properly get this info, stop using placeholders

                const auto declaring_class_name = ClassUtils::MakeDotClassName(call_info.caller_type->GetClassName());
                const auto method_name = call_info.invokable_name;
                const auto file_name = u"dummy-file.java";
                const auto line_no = 69;
                JAVM_LOG("[java.lang.Throwable.getStackTraceElement] called - stack[%d] -> class_name: '%s', method_name: '%s', file_name: '%s', line_no: %d", idx, str::ToUtf8(declaring_class_name).c_str(), str::ToUtf8(method_name).c_str(), str::ToUtf8(file_name).c_str(), line_no);

                auto declaring_class_name_v = jstr::CreateNew(declaring_class_name);
                auto method_name_v = jstr::CreateNew(method_name);
                auto file_name_v = jstr::CreateNew(file_name);
                auto line_no_v = TypeUtils::NewPrimitiveVariable<type::Integer>(line_no);
                const auto ret = stack_trace_elem_obj->CallConstructor(stack_trace_elem_v, u"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V", declaring_class_name_v, method_name_v, file_name_v, line_no_v);
                if(ret.IsInvalidOrThrown()) {
                    return ret;
                }

                return ExecutionResult::ReturnVariable(stack_trace_elem_v);
            }
    };

}