// CoolProp::add_fluids_as_JSON("HEOS", input): the user-supplied Helmholtz fluid JSON
// loader (JSONFluidLibrary::add_many: EOS terms, ancillaries, transport, expression blocks),
// then evaluates the added fluid at a few states so the parsed terms are used.
//
// Per-input isolation: the HEOS library indexes a fluid by INFO.NAME, INFO.CAS and every entry of
// INFO.ALIASES, and with OVERWRITE_FLUIDS a match replaces that fluid. So each array element's
// identity is rewritten to its harness-owned slot (cp_slot_name) before loading: NAME = slot,
// CAS = "slot-cas", ALIASES = []. Everything else in the input reaches the loader unchanged.
// Input that is not JSON goes to the loader as-is (it throws in its own parser before touching
// the library).
#include "common.h"
#include <nlohmann/json.hpp>
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_warmup(); return 0; }
static void eval(const std::string& name) {
    const std::string f = "HEOS::" + name;
    (void)CoolProp::PropsSI("D", "T", 300, "P", 101325, f);
    (void)CoolProp::PropsSI("P", "T", 280, "Q", 0, f);
    (void)CoolProp::PropsSI("V", "T", 350, "Dmolar", 100, f);
    (void)CoolProp::PropsSI("L", "T", 350, "Dmolar", 100, f);
    (void)CoolProp::PropsSI("I", "T", 270, "Q", 0, f);
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    nlohmann::json j = nlohmann::json::parse(s, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        try { CoolProp::add_fluids_as_JSON("HEOS", s); } catch (...) {}
        return 0;
    }
    if (!j.is_array()) j = nlohmann::json::array({j});
    std::vector<std::string> slots;
    for (size_t i = 0; i < j.size(); ++i) {
        auto& el = j[i];
        if (el.is_object() && el.contains("INFO") && el["INFO"].is_object()) {
            el["INFO"]["NAME"] = cp_slot_name(i);
            el["INFO"]["CAS"] = cp_slot_name(i) + "-cas";
            el["INFO"]["ALIASES"] = nlohmann::json::array();
            slots.push_back(cp_slot_name(i));
        }
    }
    try {
        CoolProp::add_fluids_as_JSON("HEOS", j.dump());
    } catch (...) {
        return 0;
    }
    for (const auto& n : slots) {
        try { eval(n); } catch (...) {}
    }
    return 0;
}
