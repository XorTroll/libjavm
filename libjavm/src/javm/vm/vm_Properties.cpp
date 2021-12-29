#include <javm/javm_VM.hpp>

namespace javm::vm {

    namespace {

        PropertyTable g_InitialSystemPropertyTable;

    }

    PropertyTable &GetInitialSystemPropertyTable() {
        return g_InitialSystemPropertyTable;
    }

    void SetInitialSystemProperty(const String &key, const String &value) {
        // Remove if already set, aka allow redefining initial properties
        auto it = g_InitialSystemPropertyTable.find(key);
        if(it != g_InitialSystemPropertyTable.end()) {
            g_InitialSystemPropertyTable.erase(it);
        }

        g_InitialSystemPropertyTable.insert(std::make_pair(key, value));
    }

}