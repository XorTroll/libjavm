
#pragma once
#include <javm/javm_Base.hpp>

namespace javm::vm {

    using PropertyTable = std::map<String, String>;

    namespace inner_impl {

        static inline PropertyTable g_system_property_table = {
            // TODO: leave these/other default values, force the dev to populate them...?
            { u"java.vm.specification.version", u"1.8-demo" },
            { u"path.separator", u":" },
            { u"file.encoding.pkg", u"sun.io" },
            { u"os.arch", u"demo-arch" },
            { u"os.name", u"Demo OS" },
            { u"os.version", u"0.1-demo" },
            { u"line.separator", u"\n" },
            { u"file.separator", u"/" },
            { u"sun.jnu.encoding", u"UTF-8" },
            { u"file.encoding", u"UTF-8" },
        };

        static inline PropertyTable &GetSystemPropertyTableImpl() {
            return g_system_property_table;
        }

    }

    static inline PropertyTable &GetSystemPropertyTable() {
        return inner_impl::GetSystemPropertyTableImpl();
    }

    static inline void SetSystemProperty(const String &name, const String &value) {
        auto &system_props = GetSystemPropertyTable();

        // Remove if already set, aka allow redefining properties
        auto it = system_props.find(name);
        if(it != system_props.end()) {
            system_props.erase(it);
        }
        system_props.insert(std::make_pair(name, value));
    }

}