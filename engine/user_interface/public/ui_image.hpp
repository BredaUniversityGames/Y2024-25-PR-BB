#pragma once
#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "ui_element.hpp"
#include <cereal/types/base_class.hpp>


class UIImage : public UIElement
{
public:
    UIImage(ResourceHandle<GPUImage> image = {}) 
        : UIElement(),_image(image) 
    {
    }

    UIImage(ResourceHandle<GPUImage> image, const glm::vec2& location, const glm::vec2& size)
        :  UIElement(),_image(image)
    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void SetImageHandle(ResourceHandle<GPUImage> image) {_image = image;}
    ResourceHandle<GPUImage> GetImageHandle() const {return _image;} 
private:
    ResourceHandle<GPUImage> _image;
};

template <class Archive>
void save(Archive& ar,const UIImage& image)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&image)));
    ar(cereal::make_nvp("image", image.GetImageHandle()));
}

template <class Archive>
void load(Archive& ar,UIImage& image)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&image)));

    ResourceHandle<GPUImage> imageHandle;
    ar(cereal::make_nvp("image",imageHandle));
    image.SetImageHandle(imageHandle);
    
}

CEREAL_REGISTER_TYPE(UIImage);

