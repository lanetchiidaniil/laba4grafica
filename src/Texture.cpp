#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include "stb_image.h"

namespace {
std::vector<unsigned char> GenerateShrekOuthouseTexture(int width, int height) {
    std::vector<unsigned char> data(static_cast<size_t>(width) * height * 3);

    const int cx = width / 2;
    const int cy = height / 2;
    const float outerR = width * 0.28f;
    const float hollowR = width * 0.22f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = (y * width + x) * 3;
            const float nx = static_cast<float>(x - cx) / std::max(1, width / 2);
            const float ny = static_cast<float>(y - cy) / std::max(1, height / 2);

            float r = 160.0f + 27.0f * std::sin((x + y * 0.75f) * 0.13f);
            float g = 118.0f + 25.0f * std::cos((x * 0.11f) + (y * 0.07f));
            float b = 90.0f + 18.0f * std::sin((x * 0.09f) - (y * 0.14f));

            const int plank = (x / 12) % 8;
            if (plank == 0 || plank == 2 || plank == 5) {
                r *= 0.92f; g *= 0.88f; b *= 0.80f;
            }

            const float dist = std::sqrt(nx * nx + ny * ny);
            const bool inDoor = dist < 0.92f && !(std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)) < 0.48f && (x - cx) > 0.0f);
            if (inDoor) {
                const float edge = std::max(0.0f, 1.0f - dist / 0.92f);
                r = 75.0f + edge * 30.0f;
                g = 58.0f + edge * 25.0f;
                b = 45.0f + edge * 20.0f;
            }

            const bool inCrescent = dist < 0.82f && (x - cx) > -0.08f * width;
            if (inCrescent && !(std::sqrt((x - (cx + width * 0.06f)) * (x - (cx + width * 0.06f)) + (y - cy) * (y - cy)) < 0.38f * width)) {
                r = 15.0f; g = 12.0f; b = 10.0f;
            }

            const float roofBand = std::abs(y - (height * 0.28f)) / (height * 0.28f);
            if (y < height * 0.18f && std::abs(x - cx) < width * 0.38f) {
                r = std::max(0.0f, r - 18.0f);
                g = std::max(0.0f, g - 16.0f);
                b = std::max(0.0f, b - 12.0f);
            }

            if (y < height * 0.22f && std::abs(x - cx) < width * 0.40f && std::abs(y - height * 0.18f) < 18.0f) {
                r = 95.0f; g = 82.0f; b = 62.0f;
            }

            if (std::abs(x - cx) < width * 0.48f && std::abs(y - cy) < height * 0.52f) {
                r *= 1.04f; g *= 1.02f; b *= 1.0f;
            }

            const float forest = std::max(0.0f, 1.0f - std::abs(nx) * 2.2f - std::abs(ny) * 1.7f);
            if (forest > 0.0f && dist > 0.9f) {
                const float green = 20.0f + forest * 90.0f;
                const float brown = 34.0f + forest * 40.0f;
                r = std::max(0.0f, r * 0.25f + green * 0.55f);
                g = std::max(0.0f, g * 0.30f + green * 0.90f);
                b = std::max(0.0f, b * 0.25f + brown * 0.35f);
            }

            data[index] = static_cast<unsigned char>(std::clamp(r, 0.0f, 255.0f));
            data[index + 1] = static_cast<unsigned char>(std::clamp(g, 0.0f, 255.0f));
            data[index + 2] = static_cast<unsigned char>(std::clamp(b, 0.0f, 255.0f));
        }
    }

    return data;
}
}

Texture::Texture(const std::string& path) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = nullptr;
    const bool fileExists = std::filesystem::exists(path);
    if (fileExists) {
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }

    if (!data) {
        width = 512;
        height = 512;
        channels = 3;
        auto generated = GenerateShrekOuthouseTexture(width, height);
        data = new unsigned char[generated.size()];
        std::copy(generated.begin(), generated.end(), data);
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (fileExists) {
        stbi_image_free(data);
    } else {
        delete[] data;
    }
}

void Texture::Bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}
