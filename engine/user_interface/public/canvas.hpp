#pragma once
#include "ui_element.hpp"
#include <cereal/types/base_class.hpp> 


struct QuadDrawInfo;

class Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size = glm::vec2(500,500))
        : UIElement()
    {
        SetScale(size);
    }
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
};

template <class Archive>
void save(Archive& ar,const Canvas& canvas)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&canvas)));
}

template <class Archive>
void load(Archive& ar,Canvas& canvas)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&canvas)));
}


CEREAL_REGISTER_TYPE(Canvas);
