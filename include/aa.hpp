#ifndef AA
#define AA

#include "vec.hpp"
#include "img.hpp"

extern const char *samplingModes[3];

namespace SamplingMode {
    const int Grid = 0;
    const int Random = 1;
    const int BlueNoise = 2;
};

void generateOffsets(Vec2* offsets, int gridSize, int sampleMode);
void visualizeOffsets(Vec2 *offsets, int gridSize, Image *img);
#endif
