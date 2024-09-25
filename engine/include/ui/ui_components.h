//
// Created by luuk on 24-9-2024.
//

#pragma once
#include "pch.hpp"

struct UITransformComponent
{
	glm::vec2 Translation {};
	glm::vec2 Scale {};
};


struct UITextComponent 
{
	std::string m_Text = "placeholder";

	int m_FontSize = 10;
	int m_CharacterDistance = 2;
	
};

struct UIButtonComponent
{
	enum class ButtonState
	{
		NORMAL,
		HOVERED,
		PRESSED
	} m_State = ButtonState::NORMAL;
	
	ResourceHandle<Image> m_NormalImage = {};
	ResourceHandle<Image> m_HoveredImage = {};
	ResourceHandle<Image> m_PressedImage = {};
	
	std::function<void()> m_OnBeginHoverCallBack {};
	std::function<void()> m_OnMouseDownCallBack {};
};