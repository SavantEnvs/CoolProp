// UNIFACLibrary::UNIFACParameterLibrary::populate(groups, interactions, decompositions): the VTPR
// backend's UNIFAC parameter loader. VTPRBackend::LoadLibrary reads these three documents from the
// user-supplied VTPR_UNIFAC_PATH/{group_data,interaction_parameters,decompositions}.json with no
// schema applied. Whatever loads is then read back (get_group / get_interaction_parameters /
// get_component) and every loaded component is put in a UNIFAC::UNIFACMixture whose activity
// coefficients are evaluated, the way the VTPR backend uses the library.
//
// Input: the three documents separated by '\n' (groups \n interactions \n decompositions).
//
// Per-input isolation: the library and the mixture are locals, fresh for every input; nothing
// global is written (VTPR_ALWAYS_RELOAD_LIBRARY and the other config keys stay at their defaults).
#include "common.h"
#include <nlohmann/json.hpp>
#include "Backends/Cubics/UNIFAC.h"
#include "Backends/Cubics/UNIFACLibrary.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_init(); return 0; }

// Integer fields of each object in a JSON array document (missing/mistyped ones are skipped).
static std::vector<int> ints(const std::string& doc, const char* key) {
    std::vector<int> out;
    nlohmann::json j = nlohmann::json::parse(doc, nullptr, /*allow_exceptions=*/false);
    if (!j.is_array()) return out;
    for (const auto& el : j)
        if (el.is_object() && el.contains(key) && el.at(key).is_number_integer()) out.push_back(el.at(key).get<int>());
    return out;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    std::vector<std::string> f = cp_split(s, 3);
    if (f.size() != 3) return -1;  // the loader takes exactly three documents
    UNIFACLibrary::UNIFACParameterLibrary lib;
    try {
        lib.populate(f[0], f[1], f[2]);
    } catch (...) {
        return 0;
    }
    for (int sgi : ints(f[0], "sgi")) {
        try { if (lib.has_group(sgi)) (void)lib.get_group(sgi); } catch (...) {}
    }
    std::vector<int> m1 = ints(f[1], "mgi1"), m2 = ints(f[1], "mgi2");
    for (size_t i = 0; i < m1.size() && i < m2.size(); ++i) {
        try { (void)lib.get_interaction_parameters(m1[i], m2[i]); } catch (...) {}
        try { (void)lib.get_interaction_parameters(m2[i], m1[i]); } catch (...) {}
    }
    std::vector<std::string> names;
    nlohmann::json d = nlohmann::json::parse(f[2], nullptr, /*allow_exceptions=*/false);
    if (d.is_array())
        for (const auto& el : d)
            if (el.is_object() && el.contains("name") && el.at("name").is_string()) names.push_back(el.at("name").get<std::string>());
    if (names.empty()) return 0;
    try {
        UNIFAC::UNIFACMixture mix(lib, 1.0);
        mix.set_components("name", names);
        mix.set_interaction_parameters();
        std::vector<double> z(names.size(), 1.0 / static_cast<double>(names.size()));
        mix.set_mole_fractions(z);
        for (double T : {250.0, 307.0, 400.0}) {
            mix.set_temperature(T);
            for (std::size_t i = 0; i < names.size(); ++i) (void)mix.ln_gamma_R(1.0 / T, i, 0);
            std::vector<double> gamma(names.size());
            mix.activity_coefficients(1.0 / T, z, gamma);
        }
    } catch (...) {
    }
    return 0;
}
