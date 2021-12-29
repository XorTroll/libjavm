#include <javm/javm_VM.hpp>

namespace javm::native {

    namespace {

        struct NativeLocation {
            String class_name;
            String name;
            String descriptor;
        };

        template<typename Fn>
        using NativeTable = std::vector<std::pair<NativeLocation, Fn>>;

        NativeTable<NativeInstanceMethod> g_NativeMethodList;
        NativeTable<NativeClassMethod> g_NativeStaticFnList;
        vm::Monitor g_NativeLock;

        bool SameLocation(const NativeLocation a, const NativeLocation b) {
            if(vm::EqualClassNames(a.class_name, b.class_name)) {
                if((a.name == b.name) && (a.descriptor == b.descriptor)) {
                    return true;
                }
            }

            return false;
        }

        void RegisterNativeInstanceMethod(const NativeLocation method_location, NativeInstanceMethod method) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(auto &[location, cur_method] : g_NativeMethodList) {
                if(SameLocation(location, method_location)) {
                    // Change the currently registered native method
                    cur_method = method;
                    return;
                }
            }
            g_NativeMethodList.push_back({ method_location, method });
        }

        bool HasNativeInstanceMethod(const NativeLocation method_location) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(const auto &[location, _method] : g_NativeMethodList) {
                if(SameLocation(location, method_location)) {
                    return true;
                }
            }
            return false;
        }

        NativeInstanceMethod FindNativeInstanceMethod(const NativeLocation method_location) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(const auto &[location, method] : g_NativeMethodList) {
                if(SameLocation(location, method_location)) {
                    return method;
                }
            }
            return nullptr;
        }

        void RegisterNativeClassMethod(const NativeLocation fn_location, NativeClassMethod fn) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(auto &[location, cur_fn] : g_NativeStaticFnList) {
                if(SameLocation(location, fn_location)) {
                    // Change the currently registered native fn
                    cur_fn = fn;
                    return;
                }
            }
            g_NativeStaticFnList.push_back({ fn_location, fn });
        }

        bool HasNativeClassMethod(const NativeLocation fn_location) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(const auto &[location, _fn] : g_NativeStaticFnList) {
                if(SameLocation(location, fn_location)) {
                    return true;
                }
            }
            return false;
        }

        NativeClassMethod FindNativeClassMethod(const NativeLocation fn_location) {
            vm::ScopedMonitorLock lk(g_NativeLock);

            for(const auto &[location, fn] : g_NativeStaticFnList) {
                if(SameLocation(location, fn_location)) {
                    return fn;
                }
            }
            return nullptr;
        }

    }

    void RegisterNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor, NativeInstanceMethod method) {
        RegisterNativeInstanceMethod({ vm::MakeSlashClassName(class_name), method_name, method_descriptor }, method);
    }

    bool HasNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor) {
        return HasNativeInstanceMethod({ vm::MakeSlashClassName(class_name), method_name, method_descriptor });
    }

    NativeInstanceMethod FindNativeInstanceMethod(const String &class_name, const String &method_name, const String &method_descriptor) {
        return FindNativeInstanceMethod({ vm::MakeSlashClassName(class_name), method_name, method_descriptor });
    }

    void RegisterNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor, NativeClassMethod fn) {
        RegisterNativeClassMethod({ vm::MakeSlashClassName(class_name), fn_name, fn_descriptor }, fn);
    }

    bool HasNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor) {
        return HasNativeClassMethod({ vm::MakeSlashClassName(class_name), fn_name, fn_descriptor });
    }

    NativeClassMethod FindNativeClassMethod(const String &class_name, const String &fn_name, const String &fn_descriptor) {
        return FindNativeClassMethod({ vm::MakeSlashClassName(class_name), fn_name, fn_descriptor });
    }
    
}