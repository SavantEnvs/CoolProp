// CoolProp::add_fluids_as_JSON("INCOMP", input): incompressible-liquid JSON loader
// (coefficient matrices, polynomial/exponential forms), then INCOMP:: evaluations.
//
// Per-input isolation: the INCOMP library keys a fluid by "name" and replaces it in place when the
// name exists. Each array element's name is rewritten to a harness-owned slot before loading;
// everything else reaches the loader unchanged. The library refuses to replace a pure fluid with a
// solution (or the reverse) under the same name, so every array index has two slots,
// "fuzzslot<i>" and "fuzzslot<i>b": when the loader refuses a slot for that reason the element
// moves to the other one, which then holds (or is empty for) the other classification. Which of the
// two a fluid lands in can depend on history; the data loaded and evaluated never does.
// Input that is not JSON goes to the loader as-is (it throws in its own parser first).
#include "common.h"
#include <nlohmann/json.hpp>
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_warmup(); return 0; }
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    nlohmann::json j = nlohmann::json::parse(s, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        try { CoolProp::add_fluids_as_JSON("INCOMP", s); } catch (...) {}
        return 0;
    }
    if (!j.is_array()) j = nlohmann::json::array({j});
    std::vector<int> idx;                 // array index of each renamed element
    std::vector<bool> alt(j.size(), false);
    auto slot = [&](size_t i) { return cp_slot_name(i) + (alt[i] ? "b" : ""); };
    for (size_t i = 0; i < j.size(); ++i)
        if (j[i].is_object()) idx.push_back(static_cast<int>(i));
    bool added = false;
    for (size_t attempt = 0; attempt <= idx.size() && !added; ++attempt) {
        for (int i : idx) j[i]["name"] = slot(i);
        try {
            CoolProp::add_fluids_as_JSON("INCOMP", j.dump());
            added = true;
        } catch (const std::exception& e) {
            // Only the pure/solution slot clash is retried, on the element it names.
            const std::string m = e.what();
            if (m.find("pure/solution classification differs") == std::string::npos) return 0;
            bool moved = false;
            for (int i : idx)
                if (!alt[i] && m.find("[" + slot(i) + "]") != std::string::npos) { alt[i] = true; moved = true; break; }
            if (!moved) return 0;
        } catch (...) {
            return 0;
        }
    }
    if (!added) return 0;
    for (int i : idx) {
        const std::string n = "INCOMP::" + slot(i);
        for (const char* out : {"D", "C", "L", "V", "Hmass", "Smass", "Tfreeze"}) {
            try { (void)CoolProp::PropsSI(out, "T", 300, "P", 101325, n); } catch (...) {}
            try { (void)CoolProp::PropsSI(out, "T", 300, "P", 101325, n + "-20%"); } catch (...) {}
            try { (void)CoolProp::PropsSI(out, "T", 300, "P", 101325, n + "[0.2]"); } catch (...) {}
        }
    }
    return 0;
}
