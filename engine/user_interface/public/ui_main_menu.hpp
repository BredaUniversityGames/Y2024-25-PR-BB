#pragma once

#include "ui_core.hpp"

class Font;
class GraphicsContext;
class MainMenuCanvas : public Canvas
{
public:
    MainMenuCanvas(const glm::vec2& size, const GraphicsContext& context, ResourceHandle<Font> font);
};