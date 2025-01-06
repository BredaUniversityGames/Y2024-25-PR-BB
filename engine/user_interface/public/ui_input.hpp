#pragma once

bool IsMouseInsideBoundary(const glm::vec2& mousePos, const glm::vec2& location, const glm::vec2& scale)
{
    return mousePos.x > location.x
        && mousePos.x < location.x + scale.x
        && mousePos.y > location.y
        && mousePos.y < location.y + scale.y;
}

class UIInputHandler{
    public:
       
    bool anyElementHasKeyFocus() 
    void UpdateFocusedElement()
    
    
    
}