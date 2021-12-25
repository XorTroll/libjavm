
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

    class Thread {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.lang.Thread.registerNatives] called");
                return ExecutionResult::Void();
            }
            
            static ExecutionResult currentThread(const std::vector<Ptr<Variable>> &param_vars) {
                auto thread_v = ThreadUtils::GetCurrentThreadInstance();
                JAVM_LOG("[java.lang.Thread.currentThread] called");

                return ExecutionResult::ReturnVariable(thread_v);
            }

            static ExecutionResult setPriority0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                auto prio_v = param_vars[0];
                const auto prio = prio_v->GetValue<type::Integer>();
                JAVM_LOG("[java.lang.Thread.setPriority0] called - priority: %d", prio);

                auto thread_obj = this_var->GetAs<type::ClassInstance>();
                auto eetop_v = thread_obj->GetField(u"eetop", u"J");
                const auto eetop = eetop_v->GetValue<type::Long>();

                native::SetThreadPriority(eetop, prio);

                return ExecutionResult::Void();
            }

            static ExecutionResult isAlive(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                auto thread_obj = this_var->GetAs<type::ClassInstance>();
                const auto name_ret = thread_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
                if(name_ret.IsInvalidOrThrown()) {
                    return name_ret;
                }
                auto name_v = name_ret.ret_var;
                const auto name = jstr::GetValue(name_v);
                auto eetop_v = thread_obj->GetField(u"eetop", u"J");
                const auto eetop = eetop_v->GetValue<type::Long>();

                auto thr = ThreadUtils::GetThreadByHandle(eetop);
                if(thr) {
                    JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' Thread name: '%s', thread handle: %ld", str::ToUtf8(name).c_str(), str::ToUtf8(thr->GetThreadName()).c_str(), eetop);
                    if(thr->GetThreadObject()->IsAlive()) {
                        return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Boolean>(1));
                    }
                }
                else {
                    JAVM_LOG("[java.lang.Thread.isAlive] Java thread name: '%s' thread handle: %ld - not found (must be finished)...", str::ToUtf8(name).c_str(), eetop);
                }

                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Boolean>(0));
            }

            static ExecutionResult start0(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                auto thr_obj = this_var->GetAs<type::ClassInstance>();
                const auto name_ret = thr_obj->CallInstanceMethod(u"getName", u"()Ljava/lang/String;", this_var);
                if(name_ret.IsInvalidOrThrown()) {
                    return name_ret;
                }
                auto name_v = name_ret.ret_var;
                const auto name = jstr::GetValue(name_v);

                ThreadUtils::RegisterAndStartThread(this_var);
                JAVM_LOG("[java.lang.Thread.start0] called - thread name: '%s'", str::ToUtf8(name).c_str());
                return ExecutionResult::Void();
            }
    };

}