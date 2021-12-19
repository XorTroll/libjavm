
#pragma once
#include <javm/rt/rt_ClassSource.hpp>

namespace javm::rt {

    struct ContextBlock {
        std::vector<Ptr<ClassSource>> class_sources;
    };

    namespace inner_impl {

        static inline ContextBlock global_context_block_impl;

        static inline ContextBlock &GetInnerGlobalContextImpl() {
            return global_context_block_impl;
        }

    }

    template<typename CS>
    static inline Ptr<CS> AddClassSource(Ptr<CS> cs) {
        auto &inner_ctx = inner_impl::GetInnerGlobalContextImpl();
        static_assert(std::is_base_of_v<ClassSource, CS>, "Object must be a javm::rt::ClassSource!");
        inner_ctx.class_sources.push_back(std::dynamic_pointer_cast<ClassSource>(cs));
        return cs;
    }

    template<typename CS, typename ...Args>
    static inline Ptr<CS> CreateAddClassSource(Args &&...args) {
        static_assert(std::is_base_of_v<ClassSource, CS>, "Object must be a javm::rt::ClassSource!");
        auto cs = ptr::New<CS>(args...);
        return AddClassSource(cs);
    }

    static Ptr<vm::ClassType> LocateClassType(const String &class_name) {
        auto &inner_ctx = inner_impl::GetInnerGlobalContextImpl();
        for(auto &source: inner_ctx.class_sources) {
            auto class_ptr = source->LocateClassType(class_name);
            if(class_ptr) {
                return class_ptr;
            }
        }
        return nullptr;
    }

    static void ResetCachedClassTypes() {
        auto &inner_ctx = inner_impl::GetInnerGlobalContextImpl();
        for(auto &source: inner_ctx.class_sources) {
            source->ResetCachedClassTypes();
        }
    }

}