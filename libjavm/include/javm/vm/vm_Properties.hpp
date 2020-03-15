
#pragma once
#include <javm/javm_Base.hpp>

namespace javm::vm {

    using PropertyTable = std::map<std::string, std::string>;

    namespace inner_impl {

        static inline PropertyTable g_system_property_table = {

            // TODO
            { "java.vm.specification.version", "1.8-demo" },
            { "path.separator", ":" },
            { "file.encoding.pkg", "sun.io" },
            { "os.arch", "demo-arch" },
            { "os.name", "Demo OS" },
            { "os.version", "0.1-demo" },
            { "line.separator", "\n" },
            { "file.separator", "/" },
            { "sun.jnu.encoding", "UTF-8" },
            { "file.encoding", "UTF-8" },

        };

        static inline PropertyTable &GetSystemPropertyTableImpl() {
            return g_system_property_table;
        }

    }

    static inline PropertyTable &GetSystemPropertyTable() {
        return inner_impl::GetSystemPropertyTableImpl();
    }

    static inline void SetSystemProperty(const std::string &name, const std::string &value) {
        auto &system_props = GetSystemPropertyTable();
        // Remove if already set (allowing to redefine properties)
        auto it = system_props.find(name);
        if(it != system_props.end()) {
            system_props.erase(it);
        }
        system_props.insert(std::make_pair(name, value));
    }

}