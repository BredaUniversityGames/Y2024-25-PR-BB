#pragma once
#include "gpu_resources.hpp"
#include "ui_element.hpp"
#include <glm/glm.hpp>

class UIImageElement : public UIElement
{
public:
    UIImageElement(ResourceHandle<GPUImage> image)
        : _image(image) {};

    UIImageElement(ResourceHandle<GPUImage> image, const glm::vec2& position, const glm::vec2& size)
        : _image(image)
    {
        SetLocation(position);
        SetScale(size);
    };

    void Update(const InputManagers& inputManagers, UIInputContext& uiInputContext) override;
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void SetColor(glm::vec4 color) { _color = std::move(color); };
    const glm::vec4& GetColor() const { return _color; }

    void SetImage(ResourceHandle<GPUImage> image) { _image = image; };
    ResourceHandle<GPUImage> GetImage() { return _image; }

private:
    ResourceHandle<GPUImage> _image;
    glm::vec4 _color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
