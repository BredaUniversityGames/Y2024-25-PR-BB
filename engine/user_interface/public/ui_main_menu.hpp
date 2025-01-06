#pragma once
#include "canvas.hpp"

class GraphicsContext;
std::unique_ptr<Canvas> CreateMainMenuCanvas(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext, std::function<void()> onPlayButtonClick, MAYBE_UNUSED std::function<void()> onExitButtonClick);
