#include <glm/glm.hpp>
#include <string>

#pragma once

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

