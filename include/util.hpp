#ifndef UTIL
#define UTIL

#include <cstdlib>
#include <string>

void *alloc(size_t n);
std::string collectWordOrString(std::stringstream& in);

#endif
