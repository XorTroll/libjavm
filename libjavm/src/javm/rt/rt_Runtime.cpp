#include <javm/javm_VM.hpp>

namespace javm::rt {

    vm::ExecutionResult PrepareExecution() {
        auto main_thr_v = vm::RegisterMainThread();

        auto tg_class_type = LocateClassType(u"java/lang/ThreadGroup");

        // The system thread group calls the internal empty ctor, but the main one doesn't
        // (Java initialization is so fucky :P)
        auto system_tg_v = vm::NewClassVariable(tg_class_type);

        auto res = tg_class_type->EnsureStaticInitializerCalled();
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        auto system_tg_obj = system_tg_v->GetAs<vm::type::ClassInstance>();
        res = system_tg_obj->CallConstructor(system_tg_v, u"()V");
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        auto main_tg_v = vm::NewClassVariable(tg_class_type);

        // Set the incomplete thread's incomplete thread group
        auto main_thr_obj = main_thr_v->GetAs<vm::type::ClassInstance>();
        main_thr_obj->SetField(u"group", u"Ljava/lang/ThreadGroup;", main_tg_v);

        auto class_class_type = LocateClassType(u"java/lang/Class");
        res = class_class_type->EnsureStaticInitializerCalled();
        if(res.IsInvalidOrThrown()) {
            return res;
        }
        class_class_type->SetStaticField(u"useCaches", u"Z", vm::MakeFalse());

        auto system_class_type = LocateClassType(u"java/lang/System");

        // Call the static block of these types so that we can call the main thread groups's ctor (why does this have to be this weird)
        auto is_class_type = LocateClassType(u"java/io/InputStream");
        res = is_class_type->EnsureStaticInitializerCalled();
        if(res.IsInvalidOrThrown()) {
            return res;
        }
        auto ps_class_type = LocateClassType(u"java/io/PrintStream");
        res = ps_class_type->EnsureStaticInitializerCalled();
        if(res.IsInvalidOrThrown()) {
            return res;
        }
        auto sm_class_type = LocateClassType(u"java/lang/SecurityManager");
        res = sm_class_type->EnsureStaticInitializerCalled();
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        auto main_thr_name = vm::jutil::NewString(native::Thread::MainThreadName);

        // Now, call the main thread group's ctor
        auto main_tg_obj = main_tg_v->GetAs<vm::type::ClassInstance>();
        res = main_tg_obj->CallConstructor(main_tg_v, u"(Ljava/lang/Void;Ljava/lang/ThreadGroup;Ljava/lang/String;)V", vm::MakeNull(), system_tg_v, main_thr_name);
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        auto dbg_class_type = LocateClassType(u"sun/security/util/Debug");
        dbg_class_type->DisableStaticInitializer();

        // Now, call the main thread's ctor
        res = main_thr_obj->CallConstructor(main_thr_v, u"(Ljava/lang/ThreadGroup;Ljava/lang/String;)V", main_tg_v, main_thr_name);
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        // Special case - set UTF-8 as the default Charset
        // ** java.nio.charset.Charset.defaultCharset = new sun.nio.cs.UTF_8();
        auto utf8_cs_type = rt::LocateClassType(u"sun/nio/cs/UTF_8");
        auto cs_v = vm::NewClassVariable(utf8_cs_type, u"()V");
        
        auto cs_type = rt::LocateClassType(u"java/nio/charset/Charset");
        cs_type->SetStaticField(u"defaultCharset", u"Ljava/nio/charset/Charset;", cs_v);
        
        // ** java.lang.System.initializeSystemClass();
        res = system_class_type->CallClassMethod(u"initializeSystemClass", u"()V");
        if(res.IsInvalidOrThrown()) {
            return res;
        }

        return vm::ExecutionResult::Void();
    }

}