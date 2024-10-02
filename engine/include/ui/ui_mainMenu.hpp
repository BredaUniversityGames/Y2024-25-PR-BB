//
// Created by luuk on 30-9-2024.
//
#pragma once

#include "UserInterfaceSystem.hpp"
#include "ui_button.hpp"
#include "ui/UserInterfaceSystem.hpp"

class MainMenuCanvas : public Canvas
{
public:
    void InitElements(const VulkanBrain& brain);
};