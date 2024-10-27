//
// Created by luuk on 30-9-2024.
//
#pragma once

#include "ui_button.hpp"
#include "../public/ui_core.hpp"

class VulkanBrain;
class MainMenuCanvas : public Canvas
{
public:
    MainMenuCanvas(const VulkanBrain& brain);
};