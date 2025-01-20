#pragma once
#include "gpu_resources.hpp"
#include "ui_element.hpp"
#include <resource_manager.hpp>

class UIImage : public UIElement
{
public:
    UIImage(ResourceHandle<GPUImage> image)
        : _image(image)
    {
    }

    UIImage(ResourceHandle<GPUImage> image, const glm::vec2& location, const glm::vec2& size)
        : _image(image)
    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

private:
    ResourceHandle<GPUImage> _image;
};
