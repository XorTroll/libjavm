#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_System.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <sys/time.h>

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult System::registerNatives(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.System.registerNatives] called...");
        return ExecutionResult::Void();
    }

    ExecutionResult System::initProperties(const std::vector<Ptr<Variable>> &param_vars) {
        auto props_v = param_vars[0];
        auto props_obj = props_v->GetAs<type::ClassInstance>();

        // First set user/dev-provided properties, then set default VM ones to ensure they're set correctly
        for(const auto &[key, value] : vm::GetInitialSystemPropertyTable()) {
            JAVM_LOG("[java.lang.System.initProperties] setting provided initial property '%s' with value '%s'", str::ToUtf8(key).c_str(), str::ToUtf8(value).c_str());
            auto key_str = jutil::NewString(key);
            auto val_str = jutil::NewString(value);
            props_obj->CallInstanceMethod(u"setProperty", u"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;", props_v, key_str, val_str);
        }
        for(const auto &[key, value] : vm::InitialVmPropertyTable) {
            JAVM_LOG("[java.lang.System.initProperties] setting VM initial property '%s' with value '%s'", str::ToUtf8(key).c_str(), str::ToUtf8(value).c_str());
            auto key_str = jutil::NewString(key);
            auto val_str = jutil::NewString(value);
            props_obj->CallInstanceMethod(u"setProperty", u"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;", props_v, key_str, val_str);
        }

        return ExecutionResult::ReturnVariable(props_v);
    }

    ExecutionResult System::arraycopy(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.System.arraycopy] called");
        
        // TODO: handle invalid param types/count
        if(param_vars.size() == 5) {
            auto src_v = param_vars[0];
            auto srcpos_v = param_vars[1];
            auto dst_v = param_vars[2];
            auto dstpos_v = param_vars[3];
            auto len_v = param_vars[4];
            if(src_v->CanGetAs<VariableType::Array>()) {
                auto src = src_v->GetAs<type::Array>();
                if(srcpos_v->CanGetAs<VariableType::Integer>()) {
                    const auto srcpos = srcpos_v->GetValue<type::Integer>();
                    if(dst_v->CanGetAs<VariableType::Array>()) {
                        auto dst = dst_v->GetAs<type::Array>();
                        if(dstpos_v->CanGetAs<VariableType::Integer>()) {
                            const auto dstpos = dstpos_v->GetValue<type::Integer>();
                            if(len_v->CanGetAs<VariableType::Integer>()) {
                                const auto len = len_v->GetValue<type::Integer>();
                                // Create a temporary array, push values there, then move them to the dst array
                                std::vector<Ptr<Variable>> tmp_values;
                                tmp_values.reserve(len);
                                for(auto i = srcpos; i < (srcpos + len); i++) {
                                    tmp_values.push_back(src->GetAt(i));
                                }
                                for(auto i = 0; i < len; i++) {
                                    dst->SetAt(i + dstpos, tmp_values[i]);
                                }
                            }
                        }
                    }
                }
            }
        }

        return ExecutionResult::Void();
    }

    ExecutionResult System::setIn0(const std::vector<Ptr<Variable>> &param_vars) {
        auto stream_v = param_vars[0];
        JAVM_LOG("[java.lang.System.setIn0] called - in stream: '%s'...", str::ToUtf8(FormatVariableType(stream_v)).c_str());
        auto system_class_type = rt::LocateClassType(u"java/lang/System");
        system_class_type->SetStaticField(u"in", u"Ljava/io/InputStream;", stream_v);
        return ExecutionResult::Void();
    }

    ExecutionResult System::setOut0(const std::vector<Ptr<Variable>> &param_vars) {
        auto stream_v = param_vars[0];
        JAVM_LOG("[java.lang.System.setOut0] called - out stream: '%s'...", str::ToUtf8(FormatVariableType(stream_v)).c_str());
        auto system_class_type = rt::LocateClassType(u"java/lang/System");
        system_class_type->SetStaticField(u"out", u"Ljava/io/PrintStream;", stream_v);
        return ExecutionResult::Void();
    }

    ExecutionResult System::setErr0(const std::vector<Ptr<Variable>> &param_vars) {
        auto stream_v = param_vars[0];
        JAVM_LOG("[java.lang.System.setErr0] called - err stream: '%s'...", str::ToUtf8(FormatVariableType(stream_v)).c_str());
        auto system_class_type = rt::LocateClassType(u"java/lang/System");
        system_class_type->SetStaticField(u"err", u"Ljava/io/PrintStream;", stream_v);
        return ExecutionResult::Void();
    }

    // TODO: lib load support

    ExecutionResult System::mapLibraryName(const std::vector<Ptr<Variable>> &param_vars) {
        auto lib_v = param_vars[0];
        const auto lib = jutil::GetStringValue(lib_v);
        JAVM_LOG("[java.lang.System.mapLibraryName] called - library name: '%s'...", str::ToUtf8(lib).c_str());
        return ExecutionResult::ReturnVariable(lib_v);
    }

    ExecutionResult System::loadLibrary(const std::vector<Ptr<Variable>> &param_vars) {
        auto lib_v = param_vars[0];
        const auto lib = jutil::GetStringValue(lib_v);
        JAVM_LOG("[java.lang.System.loadLibrary] called - library name: '%s'...", str::ToUtf8(lib).c_str());
        return ExecutionResult::Void();
    }

    ExecutionResult System::currentTimeMillis(const std::vector<Ptr<Variable>> &param_vars) {
        timeval time = {};
        gettimeofday(&time, nullptr);
        const auto time_ms = time.tv_sec * 1000 + time.tv_usec / 1000;
        JAVM_LOG("[java.lang.System.currentTimeMillis] called - time ms: %ld", time_ms);
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Long>(time_ms));
    }

    ExecutionResult System::identityHashCode(const std::vector<Ptr<Variable>> &param_vars) {
        return GetObjectHashCode(param_vars[0]);
    }

}