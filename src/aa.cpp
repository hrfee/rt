#include "aa.hpp"
#include <random>

const char *samplingModes[3] = {"Grid", "Random Points", "\"Blue Noise\""};

// "offsets" must be an array of size gridSize*gridSize.
void generateOffsets(Vec2* offsets, int gridSize, int sampleMode) {
    if (sampleMode == SamplingMode::Grid) {
        float divBy = gridSize+1;
        int loopBounds = gridSize;
        float div[2] = {1.f/divBy, 1.f/divBy};
        for (int i = 0; i < loopBounds; i++) {
            for (int j = 0; j < loopBounds; j++) {
                offsets[(i*loopBounds)+j].x = (0.5 - (i+1)*div[0]);
                offsets[(i*loopBounds)+j].y = (0.5 - (j+1)*div[1]);
            }
        }
    } else if (sampleMode == SamplingMode::Random) {
        std::random_device rd;
        int seed = rd();
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> dist(-0.5, 0.5);

        for (int i = 0; i < gridSize*gridSize; i++) {
            offsets[i].x = dist(gen);
            offsets[i].y = dist(gen);
        }
    } else if (sampleMode == SamplingMode::BlueNoise) {
        generateOffsets(offsets, gridSize, SamplingMode::Grid);
        std::random_device rd;
        int seed = rd();
        std::mt19937 gen(seed);
        float divs = 1.f/float(gridSize+1);
        std::uniform_real_distribution<float> dist(-divs, divs);
        for (int i = 0; i < gridSize*gridSize; i++) {
            offsets[i].x += dist(gen);
            offsets[i].y += dist(gen);
        }
    }
    return;
}

void visualizeOffsets(Vec2 *offsets, int gridSize, Image *img) {
    clearImage(img);
}
