#include "log.hpp"

#include <glm/vec3.hpp>

template <>
struct fmt::formatter<glm::vec3> : fmt::formatter<std::string>
{
    auto format(glm::vec3 value, format_context& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}, {}, {}", value.x, value.y, value.z);
    }
};
