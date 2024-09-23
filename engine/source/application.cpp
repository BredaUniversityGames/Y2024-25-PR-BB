#include "application.hpp"

Application::Application(const CreateParameters& parameters)
    : _width(0)
    , _height(0)
    , _windowTitle(parameters.windowTitle)
    , _isFullscreen(parameters.isFullscreen)
{
}

Application::~Application() = default;
