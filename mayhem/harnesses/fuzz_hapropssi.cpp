// HumidAir::HAPropsSI with fuzzed names and values.
// Input: "Output\nName1\nValue1\nName2\nValue2\nName3\nValue3".
#include "common.h"
#include "CoolProp/HumidAirProp.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) {
    cp_fuzz_warmup();
    (void)HumidAir::HAPropsSI("W", "T", 298.15, "P", 101325, "R", 0.5);
    return 0;
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string s(reinterpret_cast<const char*>(data), size);
    auto f = cp_split(s, 7);
    if (f.size() != 7) return -1;
    (void)HumidAir::HAPropsSI(f[0], f[1], cp_num(f[2]), f[3], cp_num(f[4]), f[5], cp_num(f[6]));
    return 0;
}
