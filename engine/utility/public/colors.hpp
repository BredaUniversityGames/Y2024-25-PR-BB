#pragma once

#include <array>
#include <glm/vec3.hpp>

constexpr uint32_t Vec3ToUint32(const glm::vec3& color)
{
    uint8_t r = static_cast<uint8_t>(color.r * 255.0f);
    uint8_t g = static_cast<uint8_t>(color.g * 255.0f);
    uint8_t b = static_cast<uint8_t>(color.b * 255.0f);
    return (r << 16) | (g << 8) | b;
}

constexpr glm::vec3 Uint32ToVec3(uint32_t color)
{
    float r = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(color & 0xFF) / 255.0f;
    return glm::vec3(r, g, b);
}

enum class ColorType
{
    Red,
    Green,
    Blue,
    Yellow,
    Magenta,
    Cyan,
    White,
    Black,
    Gray,
    DarkRed,
    DarkGreen,
    DarkBlue,
    Olive,
    Purple,
    Teal,
    Orange,
    Lime,
    Violet,
    Pink,
    AquaGreen,
    SkyBlue,
    Peach,
    Brown,
    Lavender,
    Seafoam,
    DarkGray,
    Goldenrod,
    Sand,
    Plum,
    Mint,
    Salmon,
    LightGreen,
    LightBlue,
    LightRed,
    LightLime,
    LightLavender,
    BrightTeal,
    BrightMagenta,
    BrightYellow,
    Crimson,
    Emerald,
    Sapphire,
    Pistachio,
    Periwinkle,
    Rose
};

constexpr std::array<glm::vec3, 48> COLORS = {
    glm::vec3(1.0f, 0.0f, 0.0f), // Red
    glm::vec3(0.0f, 1.0f, 0.0f), // Green
    glm::vec3(0.0f, 0.0f, 1.0f), // Blue
    glm::vec3(1.0f, 1.0f, 0.0f), // Yellow
    glm::vec3(1.0f, 0.0f, 1.0f), // Magenta
    glm::vec3(0.0f, 1.0f, 1.0f), // Cyan
    glm::vec3(1.0f, 1.0f, 1.0f), // White
    glm::vec3(0.0f, 0.0f, 0.0f), // Black
    glm::vec3(0.5f, 0.5f, 0.5f), // Gray
    glm::vec3(0.5f, 0.0f, 0.0f), // Dark Red
    glm::vec3(0.0f, 0.5f, 0.0f), // Dark Green
    glm::vec3(0.0f, 0.0f, 0.5f), // Dark Blue
    glm::vec3(0.5f, 0.5f, 0.0f), // Olive
    glm::vec3(0.5f, 0.0f, 0.5f), // Purple
    glm::vec3(0.0f, 0.5f, 0.5f), // Teal
    glm::vec3(0.8f, 0.4f, 0.0f), // Orange
    glm::vec3(0.4f, 0.8f, 0.0f), // Lime
    glm::vec3(0.4f, 0.0f, 0.8f), // Violet
    glm::vec3(0.8f, 0.0f, 0.4f), // Pink
    glm::vec3(0.0f, 0.8f, 0.4f), // Aqua Green
    glm::vec3(0.0f, 0.4f, 0.8f), // Sky Blue
    glm::vec3(0.9f, 0.7f, 0.5f), // Peach
    glm::vec3(0.7f, 0.5f, 0.3f), // Brown
    glm::vec3(0.5f, 0.3f, 0.7f), // Lavender
    glm::vec3(0.3f, 0.7f, 0.5f), // Seafoam
    glm::vec3(0.2f, 0.2f, 0.2f), // Dark Gray
    glm::vec3(0.8f, 0.6f, 0.0f), // Goldenrod
    glm::vec3(0.6f, 0.4f, 0.2f), // Sand
    glm::vec3(0.4f, 0.2f, 0.6f), // Plum
    glm::vec3(0.2f, 0.6f, 0.4f), // Mint
    glm::vec3(0.9f, 0.3f, 0.3f), // Salmon
    glm::vec3(0.3f, 0.9f, 0.3f), // Light Green
    glm::vec3(0.3f, 0.3f, 0.9f), // Light Blue
    glm::vec3(1.0f, 0.5f, 0.5f), // Light Red
    glm::vec3(0.5f, 1.0f, 0.5f), // Light Lime
    glm::vec3(0.5f, 0.5f, 1.0f), // Light Lavender
    glm::vec3(0.0f, 0.7f, 0.7f), // Bright Teal
    glm::vec3(0.7f, 0.0f, 0.7f), // Bright Magenta
    glm::vec3(0.7f, 0.7f, 0.0f), // Bright Yellow
    glm::vec3(0.8f, 0.2f, 0.2f), // Crimson
    glm::vec3(0.2f, 0.8f, 0.2f), // Emerald
    glm::vec3(0.2f, 0.2f, 0.8f), // Sapphire
    glm::vec3(0.6f, 0.8f, 0.4f), // Pistachio
    glm::vec3(0.4f, 0.6f, 0.8f), // Periwinkle
    glm::vec3(0.8f, 0.4f, 0.6f), // Rose
};

constexpr glm::vec3 GetColor(ColorType color)
{
    return COLORS[static_cast<size_t>(color)];
}
