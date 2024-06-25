#include "util.hpp"

#include <cstdio>
#include <sstream>

void *alloc(size_t n) {
    void *ptr = malloc(n);
    if (ptr == NULL) {
        std::printf("Failed allocation!\n");
    }
    return ptr;
}

// Extract a word, or quote-delimited string from a stringstream. The open quote must be within the first block of non-whitespace characters.
std::string collectWordOrString(std::stringstream& in) {
    // std::printf("input: \"%s\"\n", in.str().c_str());
    char delim = '\"';
    bool inString = false;
    char c;
    char lc = ' ';
    std::string out;
    while (in.get(c)) {
        // std::printf("read '%c'\n", c);
        if (!inString && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            if (out.empty()) continue;
            else return out;
        }
        if (inString && c == delim && lc != '\\') {
            return out;
        }
        if (!inString && (c == '"' || c == '\'') && lc != '\\') {
            inString = true;
            delim = c;
            out = "";
            continue;
        }
        lc = c;
        out += c;
    }
    return out;
}
