// CoolProp::add_fluids_as_JSON("SRK", input): the cubic fluid JSON loader (schema validation,
// alpha functions incl. Mathias-Copeman), then SRK:: and PR:: evaluations of the added fluid.
//
// Per-input isolation: the cubic library keys a fluid by upper(name) (replaced under
// OVERWRITE_FLUIDS) and adds each alias to a never-pruned alias map. So each array element's
// name is rewritten to its harness-owned slot (cp_slot_name), CAS to "slot-cas" and aliases to [],
// before loading; everything else reaches the loader unchanged. Input that is not JSON goes to the
// loader as-is (it throws in its own parser before touching the library).
#include "common.h"
#include <nlohmann/json.hpp>
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_warmup(); return 0; }
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    nlohmann::json j = nlohmann::json::parse(s, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        try { CoolProp::add_fluids_as_JSON("SRK", s); } catch (...) {}
        return 0;
    }
    if (!j.is_array()) j = nlohmann::json::array({j});
    std::vector<std::string> slots;
    for (size_t i = 0; i < j.size(); ++i) {
        auto& el = j[i];
        if (!el.is_object()) continue;
        el["name"] = cp_slot_name(i);
        el["CAS"] = cp_slot_name(i) + "-cas";
        el["aliases"] = nlohmann::json::array();
        slots.push_back(cp_slot_name(i));
    }
    try {
        CoolProp::add_fluids_as_JSON("SRK", j.dump());
    } catch (...) {
        return 0;
    }
    for (const auto& n : slots) {
        for (const char* b : {"SRK::", "PR::"}) {
            try {
                (void)CoolProp::PropsSI("D", "T", 300, "P", 101325, b + n);
                (void)CoolProp::PropsSI("P", "T", 250, "Q", 0, b + n);
                (void)CoolProp::PropsSI("Hmolar", "T", 400, "P", 1e6, b + n);
            } catch (...) {
            }
        }
    }
    return 0;
}
