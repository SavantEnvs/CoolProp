// The expression DSL used inside fluid JSON: compile_block() on the JSON block form, and
// compile() on the bare formula with fixed constants/arrays; evaluates whatever compiles.
#include "common.h"
#include "CoolProp/expression/Expression.h"
#include "CoolProp/expression/ExpressionBlock.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_init(); return 0; }
static void run(const CoolProp::expression::Program& p) {
    std::vector<double> in(p.requiredInputs().size(), 300.0);
    (void)p.evaluate(in);
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    try {
        run(CoolProp::expression::compile_block(s));
    } catch (...) {
    }
    try {
        run(CoolProp::expression::compile(s, {{"a", 1.5}, {"b", -2.0}, {"Tc", 647.096}},
                                          {{"n", {1.0, 2.0, 3.0}}, {"t", {0.5, 1.0, 1.5}}}, {"T"}));
    } catch (...) {
    }
    return 0;
}
