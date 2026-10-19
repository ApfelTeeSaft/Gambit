#include "port/os/gx2/gx2_cpu_features.h"

int main() {
    const bool feature = gx2_cpu_features::has_ssse3();
    return gx2_cpu_features::has_ssse3() == feature ? 0 : 1;
}
