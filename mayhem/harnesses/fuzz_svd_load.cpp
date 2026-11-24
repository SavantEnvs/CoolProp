// SVDSurfaceSerializer::load: the zlib + msgpack SVD surface cache file format.
#include "common.h"
#include "CoolProp/sbtl/SVDSurfaceSerializer.h"
#include "CoolProp/sbtl/SVDSurface.h"
extern "C" int LLVMFuzzerInitialize(int*, char***) { cp_fuzz_init(); return 0; }
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::vector<char> buf(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + size);
    try {
        auto surf = CoolProp::sbtl::SVDSurfaceSerializer::load(buf);
        (void)CoolProp::sbtl::SVDSurfaceSerializer::save(surf);
    } catch (...) {
    }
    return 0;
}
