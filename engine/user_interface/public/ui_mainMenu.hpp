//
// Created by luuk on 30-9-2024.
//
#pragma once

#include "../public/ui_core.hpp"
#include "ui_button.hpp"

class Font;
class VulkanBrain;
class MainMenuCanvas : public Canvas
{
public:
    MainMenuCanvas(const glm::vec2& size, std::shared_ptr<GraphicsContext> context, const Font& font);
};