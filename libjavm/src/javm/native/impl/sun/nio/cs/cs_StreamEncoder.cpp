#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/nio/cs/cs_StreamEncoder.hpp>

namespace javm::native::impl::sun::nio::cs {

    using namespace vm;

    ExecutionResult StreamEncoder::forOutputStreamWriter(const std::vector<Ptr<Variable>> &param_vars) {
        auto stream_v = param_vars[0];
        auto obj_v = param_vars[1];
        auto cs_name_v = param_vars[2];
        const auto cs_name = jutil::GetStringValue(cs_name_v);

        JAVM_LOG("[sun.nio.cs.StreamEncoder.forOutputStreamWriter] called - Charset name: '%s'...", str::ToUtf8(cs_name).c_str());
        auto se_class_type = rt::LocateClassType(u"sun/nio/cs/StreamEncoder");
        auto cs_class_type = rt::LocateClassType(u"java/nio/charset/Charset");
        auto global_cs_v = cs_class_type->GetStaticField(u"defaultCharset", u"Ljava/nio/charset/Charset;");
        auto se_v = NewClassVariable(se_class_type, u"(Ljava/io/OutputStream;Ljava/lang/Object;Ljava/nio/charset/Charset;)V", stream_v, obj_v, global_cs_v);
        return ExecutionResult::ReturnVariable(se_v);
    }

}