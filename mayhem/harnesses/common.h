// Shared helpers for the CoolProp fuzz harnesses.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "CoolProp/CoolProp.h"
#include "CoolProp/Configuration.h"

// Process-global, set once and never changed by an input: re-adding a fluid / departure
// function / BIP under a name that already exists overwrites it instead of throwing.
// Inputs never get to overwrite a built-in entry with this on: the fluid-JSON harnesses rename
// every input fluid into harness-owned slots (cp_slot_name) and fuzz_departure restores the
// shipped departure-function / interaction-parameter data after every input. Overwrite is what
// lets those slots be re-filled by the next input instead of throwing "already in library".
static inline void cp_fuzz_init() {
    CoolProp::set_config_bool(OVERWRITE_FLUIDS, true);
    CoolProp::set_config_bool(OVERWRITE_DEPARTURE_FUNCTION, true);
    CoolProp::set_config_bool(OVERWRITE_BINARY_INTERACTION, true);
}

// The embedded fluid/mixture libraries are parsed on first use (about 1-2 s under ASan with
// LAZY_LOAD_SUPERANCILLARIES, see build.sh; ~30 s without): do it
// once here, outside the per-input time, so every input starts from the same loaded library.
static inline void cp_fuzz_warmup() {
    cp_fuzz_init();
    (void)CoolProp::PropsSI("T", "P", 101325, "Q", 0, "HEOS::Water");
    (void)CoolProp::PropsSI("D", "T", 250, "P", 1e6, "HEOS::Methane[0.5]&Ethane[0.5]");
    (void)CoolProp::PropsSI("D", "T", 300, "P", 1e6, "SRK::Methane");
    (void)CoolProp::PropsSI("D", "T", 300, "P", 101325, "INCOMP::MEG-20%");
}

// Split input at '\n' into exactly n fields (the last one takes the rest).
static inline std::vector<std::string> cp_split(const std::string& s, size_t n) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (out.size() + 1 < n) {
        size_t nl = s.find('\n', pos);
        if (nl == std::string::npos) break;
        out.push_back(s.substr(pos, nl - pos));
        pos = nl + 1;
    }
    out.push_back(pos <= s.size() ? s.substr(pos) : std::string());
    return out;
}

// Parse a double the way a caller reading text would (strtod on a NUL-terminated copy).
static inline double cp_num(const std::string& s) { return std::strtod(s.c_str(), nullptr); }

// The table-building backends (BICUBIC&, TTSE&, SVDSBTL&) build multi-megabyte tables under
// $HOME for tens of seconds to minutes per fluid, and REFPROP needs the proprietary NIST
// library: neither is input parsing, so those fluid strings are not exercised here.
static inline bool cp_slow_backend(const std::string& s) {
    for (const char* bad : {"BICUBIC", "TTSE", "SVDSBTL", "REFPROP", "bicubic", "ttse", "svdsbtl", "refprop"})
        if (s.find(bad) != std::string::npos) return true;
    return false;
}

// Each input's fluids go into fixed, harness-owned library slots named "fuzzslot<i>" (i = index of
// the element in the input's array; no built-in fluid has such a name). Re-adding a slot replaces
// it (OVERWRITE_FLUIDS), so an input can never replace a built-in fluid (Water, Methane, MEG, ...),
// the libraries only hold as many extra fluids as the longest array seen, and a harness evaluates
// only the slots its own input just filled: no input reads a fluid left behind by another.
static inline std::string cp_slot_name(size_t i) { return "fuzzslot" + std::to_string(i); }
