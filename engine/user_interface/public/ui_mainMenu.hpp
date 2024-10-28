//
// Created by luuk on 30-9-2024.
//
#pragma once

#include "ui_button.hpp"
#include "../public/ui_core.hpp"

class Font;
class VulkanBrain;
class MainMenuCanvas : public Canvas
{
public:
    MainMenuCanvas(const glm::vec2& size, const VulkanBrain& brain, ResourceHandle<Font> font);
};