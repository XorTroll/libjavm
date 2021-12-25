
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

namespace javm::native::impl::sun::nio::cs {

    using namespace vm;

    class StreamEncoder {
        public:
            static ExecutionResult forOutputStreamWriter(const std::vector<Ptr<Variable>> &param_vars) {
                auto stream_v = param_vars[0];
                auto obj_v = param_vars[1];
                auto cs_name_v = param_vars[2];
                const auto cs_name = jstr::GetValue(cs_name_v);
                JAVM_LOG("[sun.nio.cs.StreamEncoder.forOutputStreamWriter] called - Charset name: '%s'...", str::ToUtf8(cs_name).c_str());
                auto se_class_type = vm::inner_impl::LocateClassTypeImpl(u"sun/nio/cs/StreamEncoder");
                auto cs_class_type = vm::inner_impl::LocateClassTypeImpl(u"java/nio/charset/Charset");
                auto global_cs_v = cs_class_type->GetStaticField(u"defaultCharset", u"Ljava/nio/charset/Charset;");
                auto se_v = TypeUtils::NewClassVariable(se_class_type, u"(Ljava/io/OutputStream;Ljava/lang/Object;Ljava/nio/charset/Charset;)V", stream_v, obj_v, global_cs_v);
                return ExecutionResult::ReturnVariable(se_v);
            }
    };

}