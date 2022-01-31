#include <Windows.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>
#include <windows.storage.h>
#include <windows.applicationmodel.h>
#include <wrl.h>
#include "i_main.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "version.h"
#include "d_eventbase.h"
#include "i_sound.h"
#include "c_cvars.h"
#include "files.h"

#include <vector>

FArgs* Args;
static int retval = 0;
extern bool AppActive;
EXTERN_CVAR(Bool, i_soundinbackground);

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

ref class GameApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
	public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
	{
		return ref new GameApp();
	}
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ arguments)
{
	Args = new FArgs();
	auto argc = __argc;
	auto wargv = __wargv;
	for (int i = 0; i < argc; i++)
	{
		Args->AppendArg(FString(wargv[i]));
	}
	OutputDebugStringW(Windows::Storage::ApplicationData::Current->LocalFolder->Path->Data());
	OutputDebugStringW(GetCommandLineW());
	progdir = Windows::ApplicationModel::Package::Current->InstalledPath->Data();
	progdir += '\\';
	SetCurrentDirectoryW(Windows::Storage::ApplicationData::Current->LocalFolder->Path->Data());
	if (FileExists(".\\args.txt"))
	{
		FileReader reader;
		if (reader.OpenFile(".\\args.txt"))
		{
			TArray<FString> fileArgs;
			auto filetext = reader.Read();
			filetext.Push(0);
			auto filetextstr = FString(filetext);
			filetextstr.Split(fileArgs, "\r\n", FString::TOK_SKIPEMPTY);
			for (int i = 0; i < fileArgs.Size(); i++)
			{
				Args->AppendArg(fileArgs[i]);
			}
			reader.Close();
			DeleteFile(L".\\args.txt");
		}
	}
	auto appSource = ref new GameApplicationSource();
	Windows::ApplicationModel::Core::CoreApplication::Run(appSource);
	return retval;
}

GameApp::GameApp()
: m_windowClosed(false), m_windowVisible(true)
{

}

void GameApp::Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &GameApp::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &GameApp::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &GameApp::OnResuming);
}

int GameMain();

void GameApp::Run()
{
	retval = GameMain();
	CoreApplication::Exit();
}

void GameApp::OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
	FString argkind;
	argkind.Format("%d\n", args->Kind);
	if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::CommandLineLaunch)
	{
		auto cmdargs = safe_cast<Windows::ApplicationModel::Activation::ICommandLineActivatedEventArgs^>(args);
		std::wstring cmdstr;
		bool quote = false;
		for (unsigned int i = 0; i < cmdargs->Operation->Arguments->Length(); i++)
		{
			if (cmdargs->Operation->Arguments->Data()[i] == L' ' && !quote)
			{
				Args->AppendArg(FString(cmdstr.c_str()));
				cmdstr.clear();
				continue;
			}
			cmdstr.push_back(cmdargs->Operation->Arguments->Data()[i]);
			if (cmdargs->Operation->Arguments->Data()[i] == L'"')
			{
				auto prevquote = quote;
				quote ^= true;
				if (prevquote == true)
				{
					Args->AppendArg(FString(cmdstr.c_str()));
					cmdstr.clear();
					continue;
				}
			}
		}
	}
	OutputDebugStringW(argkind.WideString().c_str());
	CoreWindow::GetForCurrentThread()->Activate();
}