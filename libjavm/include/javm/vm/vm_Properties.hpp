
#pragma once
#include <javm/javm_Base.hpp>

namespace javm::vm {

    using PropertyTable = std::map<String, String>;

    namespace inner_impl {

        static inline PropertyTable g_initial_system_property_table;

        static inline PropertyTable &GetInitialSystemPropertyTableImpl() {
            return g_initial_system_property_table;
        }

    }

    const PropertyTable InitialVmPropertyTable = {
        // TODO: any more initial properties only the VM itself should define?
        { u"java.vm.specification.version", u"1.8" }
    };

    static inline PropertyTable &GetInitialSystemPropertyTable() {
        return inner_impl::GetInitialSystemPropertyTableImpl();
    }

    static inline void SetInitialSystemProperty(const String &key, const String &value) {
        auto &initial_system_props = GetInitialSystemPropertyTable();

        // Remove if already set, aka allow redefining initial properties
        auto it = initial_system_props.find(key);
        if(it != initial_system_props.end()) {
            initial_system_props.erase(it);
        }

        initial_system_props.insert(std::make_pair(key, value));
    }

    static inline void SetInitialSystemProperties(const PropertyTable &props) {
        for(const auto &[key, value] : props) {
            SetInitialSystemProperty(key, value);
        }
    }

}