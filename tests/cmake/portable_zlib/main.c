#include <zlib.h>

int main(void) {
    const char *version = zlibVersion();
    return version == 0 || version[0] == '\0';
}
