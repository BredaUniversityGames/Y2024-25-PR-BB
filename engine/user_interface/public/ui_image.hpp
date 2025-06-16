#pragma once
#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "ui_element.hpp"

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

    void SetImage(ResourceHandle<GPUImage> image) { _image = image; }
    ResourceHandle<GPUImage> GetImage() const { return _image; }

private:
    ResourceHandle<GPUImage> _image;
};
