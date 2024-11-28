#pragma once

#include "canvas.hpp"
#include "ui_button.hpp"

class UIFont;
class GraphicsContext;
class MainMenuCanvas : public Canvas
{
public:
    MainMenuCanvas(const glm::vec2& size, std::shared_ptr<GraphicsContext>& context, const std::shared_ptr<UIFont>& font);
};