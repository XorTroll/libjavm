
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <functional>
#include <map>
#include <javm/vm/vm_Sync.hpp>

namespace javm::native {

    using NativeInstanceMethod = std::function<vm::ExecutionResult(Ptr<vm::Variable>, std::vector<Ptr<vm::Variable>>)>;
    using NativeClassMethod = std::function<vm::ExecutionResult(std::vector<Ptr<vm::Variable>>)>;

    namespace inner_impl {

        static inline bool g_native_registered = false;

        static inline bool IsNativeRegistered() {
            return g_native_registered;
        }

        static inline void NotifyNativeRegistered() {
            g_native_registered = true;
        }

        struct NativeLocation {
            std::string class_name;
            std::string name;
            std::string descriptor;
        };

        template<typename Fn>
        using NativeTable = std::vector<std::pair<NativeLocation, Fn>>;

        static inline NativeTable<NativeInstanceMethod> g_native_method_table;
        static inline NativeTable<NativeClassMethod> g_native_static_fn_table;
        static inline vm::Monitor g_native_lock;

        static inline vm::ExecutionResult EmptyNativeInstanceMethod(Ptr<vm::Variable> this_var, std::vector<Ptr<vm::Variable>> param_vars) {
            vm::ScopedMonitorLock lk(g_native_lock);
            return vm::ExecutionResult::InvalidState();
        }

        static inline vm::ExecutionResult EmptyNativeClassMethod(std::vector<Ptr<vm::Variable>> param_vars) {
            vm::ScopedMonitorLock lk(g_native_lock);
            return vm::ExecutionResult::InvalidState();
        }

        static inline bool SameLocation(NativeLocation a, NativeLocation b) {
            vm::ScopedMonitorLock lk(g_native_lock);
            if(vm::ClassUtils::EqualClassNames(a.class_name, b.class_name)) {
                if(a.name == b.name) {
                    if(a.descriptor == b.descriptor) {
                        return true;
                    }
                }
            }
            return false;
        }

        static inline void RegisterNativeInstanceMethod(NativeLocation method_location, NativeInstanceMethod method) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, _method] : g_native_method_table) {
                if(SameLocation(location, method_location)) {
                    // A native method for this is already present
                    return;
                }
            }
            g_native_method_table.push_back(std::make_pair(method_location, method));
        }

        static inline bool HasNativeInstanceMethod(NativeLocation method_location) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, _method] : g_native_method_table) {
                if(SameLocation(location, method_location)) {
                    return true;
                }
            }
            return false;
        }

        static inline NativeInstanceMethod FindNativeInstanceMethod(NativeLocation method_location) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, method] : g_native_method_table) {
                if(SameLocation(location, method_location)) {
                    return method;
                }
            }
            return &EmptyNativeInstanceMethod;
        }

        static inline void RegisterNativeClassMethod(NativeLocation fn_location, NativeClassMethod fn) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, _fn] : g_native_static_fn_table) {
                if(SameLocation(location, fn_location)) {
                    // A native method for this is already present
                    return;
                }
            }
            g_native_static_fn_table.push_back(std::make_pair(fn_location, fn));
        }

        static inline bool HasNativeClassMethod(NativeLocation fn_location) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, _fn] : g_native_static_fn_table) {
                if(SameLocation(location, fn_location)) {
                    return true;
                }
            }
            return false;
        }

        static inline NativeClassMethod FindNativeClassMethod(NativeLocation fn_location) {
            vm::ScopedMonitorLock lk(g_native_lock);
            for(auto &[location, fn] : g_native_static_fn_table) {
                if(SameLocation(location, fn_location)) {
                    return fn;
                }
            }
            return &EmptyNativeClassMethod;
        }

    }

    static inline void RegisterNativeInstanceMethod(const std::string &class_name, const std::string &method_name, const std::string &method_descriptor, NativeInstanceMethod method) {
        inner_impl::RegisterNativeInstanceMethod({ vm::ClassUtils::MakeSlashClassName(class_name), method_name, method_descriptor }, method);
    }

    static inline bool HasNativeInstanceMethod(const std::string &class_name, const std::string &method_name, const std::string &method_descriptor) {
        return inner_impl::HasNativeInstanceMethod({ vm::ClassUtils::MakeSlashClassName(class_name), method_name, method_descriptor });
    }

    static inline NativeInstanceMethod FindNativeInstanceMethod(const std::string &class_name, const std::string &method_name, const std::string &method_descriptor) {
        return inner_impl::FindNativeInstanceMethod({ vm::ClassUtils::MakeSlashClassName(class_name), method_name, method_descriptor });
    }

    static inline void RegisterNativeClassMethod(const std::string &class_name, const std::string &fn_name, const std::string &fn_descriptor, NativeClassMethod fn) {
        inner_impl::RegisterNativeClassMethod({ vm::ClassUtils::MakeSlashClassName(class_name), fn_name, fn_descriptor }, fn);
    }

    static inline bool HasNativeClassMethod(const std::string &class_name, const std::string &fn_name, const std::string &fn_descriptor) {
        return inner_impl::HasNativeClassMethod({ vm::ClassUtils::MakeSlashClassName(class_name), fn_name, fn_descriptor });
    }

    static inline NativeClassMethod FindNativeClassMethod(const std::string &class_name, const std::string &fn_name, const std::string &fn_descriptor) {
        return inner_impl::FindNativeClassMethod({ vm::ClassUtils::MakeSlashClassName(class_name), fn_name, fn_descriptor });
    }

}