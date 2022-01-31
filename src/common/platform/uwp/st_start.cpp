/*
 ** st_start.mm
 **
 **---------------------------------------------------------------------------
 ** Copyright 2015 Alexey Lysiuk
 ** Copyright 2021 Cacodemon345
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 **
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 ** 3. The name of the author may not be used to endorse or promote products
 **    derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 ** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 ** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 ** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 ** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 ** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **---------------------------------------------------------------------------
 **
 */

#include "st_start.h"
#include "engineerrors.h"
#include <Windows.h>

int ProgressBarCurPos, ProgressBarMaxPos;
extern bool netinited;

FBasicStartupScreen::FBasicStartupScreen(int max_progress, bool show_bar)
: FStartupScreen(max_progress)
{
#if 0
	ProgressBarMaxPos = max_progress;
    FConsoleWindow::GetInstance().SetProgressBar(show_bar);
#endif
}

FBasicStartupScreen::~FBasicStartupScreen()
{
#if 0
	NetDone();
	FConsoleWindow::GetInstance().SetProgressBar(false);
#endif
}

void FBasicStartupScreen::NetInit(const char* const message, const int playerCount)
{
#if 0
	FConsoleWindow::GetInstance().NetInit(message, playerCount);
	netinited = true;
	extern void CleanProgressBar();
	CleanProgressBar();
#endif
}

void FBasicStartupScreen::NetProgress(const int count)
{
#if 0
	FConsoleWindow::GetInstance().NetProgress(count);
#endif
}

void FBasicStartupScreen::NetMessage(const char* const format, ...)
{
#if 0
	va_list args;
	va_start(args, format);

	FString message;
	message.VFormat(format, args);
	va_end(args);

	Printf("%s", message.GetChars());
#endif
}

void FBasicStartupScreen::Progress()
{
	if (CurPos < MaxPos)
	{
		++CurPos;
	}
	ProgressBarCurPos = CurPos;
	ProgressBarMaxPos = MaxPos;
#if 0	
	extern void RedrawProgressBar(int CurPos, int MaxPos);
	RedrawProgressBar(ProgressBarCurPos, ProgressBarMaxPos);
	if (FConsoleWindow::GetInstance().GetStartupType() != FConsoleWindow::StartupType::StartupTypeNormal) FConsoleWindow::GetInstance().RunLoop();
	else
	{
		static auto ticks = GetTickCount64();
		if (GetTickCount64() - ticks > 250)
		{
			ticks = GetTickCount64();
			FConsoleWindow::GetInstance().RunLoop();
		}
	}
#endif
}

void FBasicStartupScreen::NetDone()
{
#if 0
	FConsoleWindow::GetInstance().NetDone();
	extern void CleanProgressBar();
	CleanProgressBar();
#endif
}

bool FBasicStartupScreen::NetLoop(bool (*timerCallback)(void*), void* const userData)
{
	return false;
}

FStartupScreen* StartScreen;

FStartupScreen* FStartupScreen::CreateInstance(int max_progress)
{
	return new FBasicStartupScreen(max_progress, false);
}