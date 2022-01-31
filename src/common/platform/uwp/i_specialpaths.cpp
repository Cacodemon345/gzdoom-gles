#include <Windows.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>
#include <windows.storage.h>
#include <windows.applicationmodel.h>
#include "i_main.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "version.h"
#include "d_eventbase.h"
#include "i_sound.h"
#include "c_cvars.h"
#include "i_specialpaths.h"

FString M_GetAppDataPath(bool create)
{
	return FString(Windows::Storage::ApplicationData::Current->LocalFolder->Path->Data());
}

FString M_GetCachePath(bool create)
{
	FString path;
	path += M_GetAppDataPath(false) + "/cache";
	if (create)
	{
		CreatePath(path);
	}
	return path;
}

FString M_GetAutoexecPath()
{
	return M_GetAppDataPath(false) + "/autoexec.cfg";
}

FString M_GetConfigPath(bool for_reading)
{
	return M_GetAppDataPath(false) + "/" GAMENAMELOWERCASE ".ini";
}

FString M_GetDocumentsPath()
{
	CreatePath(M_GetAppDataPath(false) + "/Documents");
	return M_GetAppDataPath(false) + "/Documents";
}

FString M_GetDemosPath()
{
	CreatePath(M_GetAppDataPath(false) + "/Demos");
	return M_GetAppDataPath(false) + "/Demos";
}

FString M_GetSavegamesPath()
{
	return M_GetDocumentsPath();
}

FString M_GetScreenshotsPath()
{
	return M_GetDocumentsPath();
}

FString M_GetNormalizedPath(const char* path)
{
	std::wstring wpath = WideString(path);
	wchar_t buffer[MAX_PATH];
	GetFullPathNameW(wpath.c_str(), MAX_PATH, buffer, nullptr);
	FString result(buffer);
	FixPathSeperator(result);
	return result;
}

TArray<FString> I_GetSteamPath(void) { return TArray<FString>(); }
TArray<FString> I_GetGogPaths(void) { return TArray<FString>(); }