#include "util.hpp"

#include <cstdio>

void *alloc(size_t n) {
    void *ptr = malloc(n);
    if (ptr == NULL) {
        std::printf("Failed allocation!\n");
    }
    return ptr;
}
