#pragma once

void CHECKRESULT_fn(FMOD_RESULT result, MAYBE_UNUSED const char* file, int line);
#define CHECKRESULT(result) CHECKRESULT_fn(result, __FILE__, __LINE__)

void StartFMODDebugLogger();