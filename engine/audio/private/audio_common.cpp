#include "audio_common.hpp"

#include <fmod_common.h>

inline FMOD_VECTOR GLMToFMOD(const glm::vec3& v)
{
    return FMOD_VECTOR(v.x, v.y, v.z);
}