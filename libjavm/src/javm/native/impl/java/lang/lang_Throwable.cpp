#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_Throwable.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    namespace {

        Ptr<Variable> CreateCallInfoStackTraceElement(const CallInfo &call_info) {
            auto stack_trace_elem_v = NewClassVariable(rt::LocateClassType(u"java/lang/StackTraceElement"));
            auto stack_trace_elem_obj = stack_trace_elem_v->GetAs<type::ClassInstance>();

            const auto declaring_class_name = MakeDotClassName(call_info.caller_type->GetClassName());
            const auto method_name = call_info.invokable_name;
            const auto file_name = call_info.caller_type->GetSourceFile();

            const auto line_no_table = call_info.caller_type->GetMethodLineNumberTable(call_info.invokable_name, call_info.invokable_desc);
            const auto line_no = line_no_table.FindLineNumber(call_info.code_offset);

            auto declaring_class_name_v = jutil::NewString(declaring_class_name);
            auto method_name_v = jutil::NewString(method_name);
            auto file_name_v = jutil::NewString(file_name);
            auto line_no_v = NewPrimitiveVariable<type::Integer>(line_no);
            const auto ret = stack_trace_elem_obj->CallConstructor(stack_trace_elem_v, u"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V", declaring_class_name_v, method_name_v, file_name_v, line_no_v);
            if(ret.IsInvalidOrThrown()) {
                return nullptr;
            }

            return stack_trace_elem_v;
        }

    }

    ExecutionResult Throwable::fillInStackTrace(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Throwable.fillInStackTrace] called");

        auto call_stack = GetCurrentThread()->GetInvertedCallStack();
        call_stack.erase(std::remove_if(call_stack.begin(), call_stack.end(), [&](const CallInfo &call_info) -> bool {
            return call_info.caller_sensitive;
        }), call_stack.end());
        const auto call_stack_size = call_stack.size();
        auto stack_trace_elem_arr_v = NewArrayVariable(call_stack_size, rt::LocateClassType(u"java/lang/StackTraceElement"));
        auto stack_trace_elem_arr = stack_trace_elem_arr_v->GetAs<type::Array>();
        for(u32 i = 0; i < call_stack_size; i++) {
            const auto &call_info = call_stack.at(i);
            auto stack_trace_elem_v = CreateCallInfoStackTraceElement(call_info);
            if(!stack_trace_elem_v) {
                return Throw(u"java/lang/NullPointerException");
            }

            stack_trace_elem_arr->SetAt(i, stack_trace_elem_v);
        }

        auto this_obj = this_var->GetAs<type::ClassInstance>();
        this_obj->SetField(u"stackTrace", u"[Ljava/lang/StackTraceElement;", MakeNull());
        this_obj->SetField(u"backtrace", u"Ljava/lang/Object;", stack_trace_elem_arr_v);
        return ExecutionResult::ReturnVariable(this_var);
    }

    ExecutionResult Throwable::getStackTraceDepth(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Throwable.getStackTraceDepth] called");

        auto this_obj = this_var->GetAs<type::ClassInstance>();
        auto stack_trace_elem_arr_v = this_obj->GetField(u"backtrace", u"Ljava/lang/Object;");
        auto stack_trace_elem_arr = stack_trace_elem_arr_v->GetAs<type::Array>();

        JAVM_LOG("[java.lang.Throwable.getStackTraceDepth] stack depth: %ld", stack_trace_elem_arr->GetLength());

        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(stack_trace_elem_arr->GetLength()));
    }

    ExecutionResult Throwable::getStackTraceElement(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto idx_v = param_vars[0];
        const auto idx = idx_v->GetValue<type::Integer>();

        JAVM_LOG("[java.lang.Throwable.getStackTraceElement] called - stack[%d]", idx);

        auto this_obj = this_var->GetAs<type::ClassInstance>();
        auto stack_trace_elem_arr_v = this_obj->GetField(u"backtrace", u"Ljava/lang/Object;");
        auto stack_trace_elem_arr = stack_trace_elem_arr_v->GetAs<type::Array>();

        auto stack_trace_elem_v = stack_trace_elem_arr->GetAt(idx);
        return ExecutionResult::ReturnVariable(stack_trace_elem_v);
    }

}