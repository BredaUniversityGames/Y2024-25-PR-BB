#pragma once
class Canvas;
class GraphicsContext;

std::unique_ptr<Canvas> CreateNavigationTestMenu(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext);
