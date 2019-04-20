/**
* Copyright (C) 2019 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "Dllmain.h"
#include "Logging\Logging.h"
#include "External\Hooking\Hook.h"

std::ofstream LOG;

FARPROC Direct3DCreate8_dll = nullptr;
FARPROC FakeDirect3DCreate8_dll = nullptr;

extern "C" __declspec(naked) void __stdcall FakeDirect3DCreate8()
{
	__asm jmp FakeDirect3DCreate8_dll
}

// Dll main function
bool APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	static HMODULE System32_dll = nullptr;
	static HMODULE patch_dll = nullptr;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		// Get log file path and open log file
		wchar_t pathname[MAX_PATH];
		GetModuleFileName(hModule, pathname, MAX_PATH);
		wcscpy_s(wcsrchr(pathname, '.'), MAX_PATH - wcslen(pathname), L".log");
		Logging::Open(pathname);

		// Starting
		Logging::Log() << "Starting BloodRayne 2 Patch! v" << APP_VERSION;

		// Init vars
		wchar_t path[MAX_PATH];

		// Get d3d8.dll from System32 folder
		GetSystemDirectory(path, MAX_PATH);
		wcscat_s(path, MAX_PATH, L"\\d3d8.dll");
		System32_dll = LoadLibrary(path);

		// System32 -> DxWrapper
		Logging::Log() << "Hooking System32 d3d8.dll APIs...";
		Hook::HotPatch(Hook::GetProcAddress(System32_dll, "Direct3DCreate8"), "Direct3DCreate8", Hook::GetProcAddress(hModule, "RealDirect3DCreate8"), true);

		// Load d3d8patch.dll from exe folder
		GetModuleFileName(nullptr, path, sizeof(path));
		wcscpy_s(wcsrchr(path, '\\'), MAX_PATH - wcslen(path), L"\\d3d8patch.dll");
		patch_dll = LoadLibrary(path);

		// FakeDirect3DCreate9 -> Patch
		FakeDirect3DCreate8_dll = Hook::GetProcAddress(patch_dll, "Direct3DCreate8");
	}
	break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		Hook::UnhookAll();
		if (patch_dll)
		{
			FreeLibrary(patch_dll);
		}
		if (System32_dll)
		{
			FreeLibrary(System32_dll);
		}
		break;
	}
	return true;
}
