//
// Created by luuk on 30-9-2024.
//
#pragma once

#include "ui_button.hpp"
#include "ui/ui_core.hpp"

class MainMenuCanvas : public Canvas
{
public:
    void InitElements(const VulkanBrain& brain);
};