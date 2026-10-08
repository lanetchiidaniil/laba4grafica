#pragma once

#include <string>
#include <glad/glad.h>

class Texture {
public:
    unsigned int ID{};

    explicit Texture(const std::string& path);
    void Bind(unsigned int unit = 0) const;
};
