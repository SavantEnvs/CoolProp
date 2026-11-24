// AbstractState::factory + update + outputs/derivatives with fuzzed backend/fluid choice,
// mole fractions, input pair, imposed phase and state values.
#include "common.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <memory>
#include "CoolProp/AbstractState.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_warmup(); return 0; }
static const char* kBackends[] = {"HEOS", "SRK", "PR", "IF97", "INCOMP", "PCSAFT"};
static const char* kFluids[] = {"Water", "CO2", "Methane", "Ethane", "Propane", "R134a", "Nitrogen", "Argon",
                                "n-Butane", "Ammonia", "Hydrogen", "MEG", "Methane&Ethane", "Nitrogen&Argon&Oxygen",
                                "CO2&Methane", "Propane&n-Butane"};
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider p(data, size);
    std::string be = p.PickValueInArray(kBackends);
    std::string fl = p.PickValueInArray(kFluids);
    try {
        std::unique_ptr<CoolProp::AbstractState> as(CoolProp::AbstractState::factory(be, fl));
        size_t nc = as->get_mole_fractions().size();
        if (nc == 0) nc = 1;
        if (fl.find('&') != std::string::npos) {
            std::vector<double> z(nc);
            for (auto& v : z) v = p.ConsumeFloatingPoint<double>();
            as->set_mole_fractions(z);
        } else if (be == "INCOMP") {
            as->set_mass_fractions({p.ConsumeFloatingPoint<double>()});
        }
        if (p.ConsumeBool()) as->specify_phase(static_cast<CoolProp::phases>(p.ConsumeIntegralInRange<int>(0, 8)));
        auto pair = static_cast<CoolProp::input_pairs>(p.ConsumeIntegralInRange<int>(0, CoolProp::DmassHmass_INPUTS + 8));
        double a = p.ConsumeFloatingPoint<double>(), b = p.ConsumeFloatingPoint<double>();
        as->update(pair, a, b);
        auto key = [&] { return static_cast<CoolProp::parameters>(p.ConsumeIntegralInRange<int>(0, 200)); };
        for (int i = 0; i < 4 && p.remaining_bytes() > 0; ++i) {
            try { (void)as->keyed_output(key()); } catch (...) {}
        }
        try { (void)as->first_partial_deriv(key(), key(), key()); } catch (...) {}
        try { (void)as->first_saturation_deriv(key(), key()); } catch (...) {}
        try { (void)as->fugacity_coefficient(0); } catch (...) {}
    } catch (...) {
    }
    return 0;
}
