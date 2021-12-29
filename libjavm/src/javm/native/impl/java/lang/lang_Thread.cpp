#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_Thread.hpp> 

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult Thread::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.Thread.registerNatives] called");
        return ExecutionResult::Void();
    }
    
    ExecutionResult Thread::currentThread(const std::vector<Ptr<Variable>> &param_vars) {
        auto thread_v = GetCurrentThreadVariable();
        JAVM_LOG("[java.lang.Thread.currentThread] called");

        return ExecutionResult::ReturnVariable(thread_v);
    }

    ExecutionResult Thread::setPriority0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto prio_v = param_vars[0];
        const auto prio = prio_v->GetValue<type::Integer>();
        JAVM_LOG("[java.lang.Thread.setPriority0] called - priority: %d", prio);

        auto thread_obj = this_var->GetAs<type::ClassInstance>();
        auto eetop_v = thread_obj->GetField(u"eetop", u"J");
        const auto eetop = eetop_v->GetValue<type::Long>();

        native::SetThreadPriority(eetop, prio);

        return ExecutionResult::Void();
    }

    ExecutionResult Thread::isAlive(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto thread_obj = this_var->GetAs<type::ClassInstance>();
        const auto name_ret = thread_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
        if(name_ret.IsInvalidOrThrown()) {
            return name_ret;
        }
        auto name_v = name_ret.var;
        const auto name = jutil::GetStringValue(name_v);

        auto eetop_v = thread_obj->GetField(u"eetop", u"J");
        const auto eetop = eetop_v->GetValue<type::Long>();

        auto accessor = GetThreadByHandle(eetop);
        if(accessor) {
            JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' Thread name: '%s', thread handle: %ld", str::ToUtf8(name).c_str(), str::ToUtf8(accessor->GetThreadName()).c_str(), eetop);
            if(accessor->GetThreadObject()->IsAlive()) {
                return ExecutionResult::ReturnVariable(MakeTrue());
            }
        }
        else {
            JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' thread handle: %ld - not found (must be finished)...", str::ToUtf8(name).c_str(), eetop);
        }

        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Thread::start0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        auto thr_obj = this_var->GetAs<type::ClassInstance>();
        const auto name_ret = thr_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
        if(name_ret.IsInvalidOrThrown()) {
            return name_ret;
        }
        auto name_v = name_ret.var;
        const auto name = jutil::GetStringValue(name_v);

        RegisterAndStartThread(this_var);
        JAVM_LOG("[java.lang.Thread.start0] called - thread name: '%s'", str::ToUtf8(name).c_str());
        return ExecutionResult::Void();
    }

}