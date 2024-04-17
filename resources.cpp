#include "resources.h"

void exit_error(const char *const msg) {
    std::cerr << msg << std::endl;
    exit(-1);
}