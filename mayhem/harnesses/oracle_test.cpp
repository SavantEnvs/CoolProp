// Known-answer functional checks of CoolProp's public API, built with the project's normal
// flags. Prints one "PASS <name>" / "FAIL <name> got=<v>" line per check; test.sh counts them.
#include <cmath>
#include <cstdio>
#include <string>
#include "CoolProp/CoolProp.h"
#include "CoolProp/HumidAirProp.h"
#include "CoolProp/expression/Expression.h"
static int fails = 0;
static void check(const char* name, double got, double want, double reltol) {
    bool ok = std::isfinite(got) && std::fabs(got - want) <= reltol * std::fabs(want);
    std::printf("%s %s got=%.10g want=%.10g\n", ok ? "PASS" : "FAIL", name, got, want);
    if (!ok) ++fails;
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    check("water_nbp", CoolProp::PropsSI("T", "P", 101325, "Q", 0, "Water"), 373.1242958, 1e-6);
    check("co2_tcrit", CoolProp::PropsSI("Tcrit", "", 0, "", 0, "CO2"), 304.1282, 1e-6);
    check("water_rho_300K", CoolProp::PropsSI("D", "T", 300, "P", 101325, "HEOS::Water"), 996.5569, 1e-5);
    check("r134a_psat_0C", CoolProp::PropsSI("P", "T", 273.15, "Q", 0, "R134a"), 292803.1823, 1e-6);
    check("srk_methane_rho", CoolProp::PropsSI("D", "T", 300, "P", 1e6, "SRK::Methane"), 6.538960306, 1e-5);
    check("if97_water_h", CoolProp::PropsSI("H", "T", 500, "P", 1e7, "IF97::Water"), 977213.9101, 1e-6);
    check("incomp_meg_rho", CoolProp::PropsSI("D", "T", 300, "P", 101325, "INCOMP::MEG-20%"), 1021.762289, 1e-5);
    check("humidair_w", HumidAir::HAPropsSI("W", "T", 298.15, "P", 101325, "R", 0.5), 0.009925739296, 1e-4);
    check("mix_rho", CoolProp::PropsSI("Dmolar", "T", 250, "P", 1e6, "HEOS::Methane[0.5]&Ethane[0.5]"), 520.4367193, 1e-4);
    auto p = CoolProp::expression::compile("a*T + sum(i: n[i])", {{"a", 2.0}}, {{"n", {1.0, 5.0}}}, {"T"});
    check("expression_eval", p.evaluate({10.0}), 26.0, 1e-12);
    bool bad = false;
    try {
        CoolProp::add_fluids_as_JSON("HEOS", "{not json");
    } catch (...) {
        bad = true;
    }
    check("bad_json_rejected", bad ? 1.0 : 0.0, 1.0, 0.0);
    return fails ? 1 : 0;
}
