#pragma once
#include "canvas.hpp"
class GraphicsContext;

std::unique_ptr<Canvas> CreateNavigationTestMenu(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext);

std::unique_ptr<Canvas> CreateMainMenu(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext);
