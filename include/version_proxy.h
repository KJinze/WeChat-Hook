#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void InitRealDll();
void CustomInit(HMODULE hModule);

extern "C" {
extern void* g_ilink_wrapper_exports[8];
}
