// PropsSI / PhaseSI with fuzzed output, input names, values and fluid/backend string
// (backend parsing, fraction parsing, factory options, number parsing, flash routines).
// Input: "Output\nName1\nValue1\nName2\nValue2\nFluid".
#include "common.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_warmup(); return 0; }
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    auto f = cp_split(s, 6);
    if (f.size() != 6) return -1;
    if (cp_slow_backend(f[5])) return -1;
    double v1 = cp_num(f[2]), v2 = cp_num(f[4]);
    (void)CoolProp::PropsSI(f[0], f[1], v1, f[3], v2, f[5]);
    (void)CoolProp::PhaseSI(f[1], v1, f[3], v2, f[5]);
    (void)CoolProp::Props1SI(f[5], f[0]);
    return 0;
}
