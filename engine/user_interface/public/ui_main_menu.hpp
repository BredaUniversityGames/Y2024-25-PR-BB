#pragma once
#include "canvas.hpp"

class GraphicsContext;

std::unique_ptr<Canvas> CreateNavigationTestCanvas(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext);
