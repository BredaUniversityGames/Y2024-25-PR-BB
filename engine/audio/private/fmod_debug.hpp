#pragma once

#include "common.hpp"
#include "fmod_common.h"

void FMOD_CHECKRESULT_fn(FMOD_RESULT result, MAYBE_UNUSED const char* file, int line);
#define FMOD_CHECKRESULT(result) FMOD_CHECKRESULT_fn(result, __FILE__, __LINE__)

void StartFMODDebugLogger();