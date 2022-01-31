#include "tarray.h"
#include "i_main.h"
#include "m_argv.h"
#include "v_video.h"

#include "d_eventbase.h"
#include "d_gui.h"
#include "c_buttons.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "common/console/keydef.h"
#include "utf8.h"
#include "keydef.h"
#include "i_interface.h"
#include "engineerrors.h"
#include "i_interface.h"
#include "m_joy.h"
#include <windows.ui.core.h>
#include <windows.system.h>

using namespace Windows::UI::Core;
using namespace Windows::System;
static bool exitevent = false;
static bool nativeMouse = true;
bool GUICapture = false;
std::vector<event_t> game_events;

CVAR(Bool, use_mouse, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

static const uint8_t Convert[256] =
{
	//  0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
		0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',   8,   9, // 0
	  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  13,   0, 'a', 's', // 1
	  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39, '`',   0,'\\', 'z', 'x', 'c', 'v', // 2
	  'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0, // 3
		0,   0,   0,   0,   0,   0,   0, '7', '8', '9', '-', '4', '5', '6', '+', '1', // 4
	  '2', '3', '0', '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 5
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 6
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 7

		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '=',   0,   0, // 8
		0, '@', ':', '_',   0,   0,   0,   0,   0,   0,   0,   0,  13,   0,   0,   0, // 9
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // A
		0,   0,   0, ',',   0, '/',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // B
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // C
		0,   0,   0,   0,   0,   0,   0,   0

};
void GameApp::OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args)
{
	exitevent = true;
}

void GameApp::OnCloseRequested(Platform::Object^ sender, Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs^ args)
{
	exitevent = true;
}

void I_SetMouseCapture()
{
	if (use_mouse) Windows::UI::Core::CoreWindow::GetForCurrentThread()->PointerCursor = nullptr;
}

void I_ReleaseMouseCapture()
{
	if (use_mouse) CoreWindow::GetForCurrentThread()->PointerCursor = ref new CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
}

void GameApp::OnMouseMoved(Windows::Devices::Input::MouseDevice^ mouseDevice, Windows::Devices::Input::MouseEventArgs^ args)
{
	if (!use_mouse) return;
	if (!nativeMouse)
	{
		PostMouseMove(args->MouseDelta.X, args->MouseDelta.Y);
	}
}

void GameApp::OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	event_t ev{};
	if (!use_mouse) return;
	if (GUICapture)
	{
		ev.type = EV_GUI_Event;
		ev.data1 = args->CurrentPoint->Position.X;
		ev.data2 = args->CurrentPoint->Position.Y;
		screen->ScaleCoordsFromWindow(ev.data1, ev.data2);
		switch (args->CurrentPoint->Properties->PointerUpdateKind)
		{
		case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
			ev.subtype = EV_GUI_LButtonDown;
			break;
		case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
			ev.subtype = EV_GUI_MButtonDown;
			break;
		case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
			ev.subtype = EV_GUI_RButtonDown;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton1Pressed:
			ev.subtype = EV_GUI_BackButtonDown;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton2Pressed:
			ev.subtype = EV_GUI_FwdButtonDown;
			break;
		}
		ev.data3 = (sender->GetKeyState(VirtualKey::Menu) == CoreVirtualKeyStates::Down ? GKM_ALT : 0)
			| (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down ? GKM_SHIFT : 0)
			| (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down ? GKM_CTRL : 0);
		game_events.push_back(ev);
	}
	else
	{
		ev.type = EV_KeyDown;
		switch (args->CurrentPoint->Properties->PointerUpdateKind)
		{
		case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
			ev.data1 = KEY_MOUSE1;
			break;
		case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
			ev.data1 = KEY_MOUSE3;
			break;
		case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
			ev.data1 = KEY_MOUSE2;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton1Pressed:
			ev.data1 = KEY_MOUSE4;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton2Pressed:
			ev.data1 = KEY_MOUSE5;
			break;
		}
		game_events.push_back(ev);
	}
}

void GameApp::OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	event_t ev{};
	if (!use_mouse) return;
	if (GUICapture)
	{
		ev.data1 = args->CurrentPoint->Position.X;
		ev.data2 = args->CurrentPoint->Position.Y;

		screen->ScaleCoordsFromWindow(ev.data1, ev.data2);

		ev.type = EV_GUI_Event;
		ev.subtype = EV_GUI_MouseMove;

		ev.data3 = (sender->GetKeyState(VirtualKey::Menu) == CoreVirtualKeyStates::Down ? GKM_ALT : 0)
			| (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down ? GKM_SHIFT : 0)
			| (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down ? GKM_CTRL : 0);
		game_events.push_back(ev);
	}
}

void GameApp::OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	event_t ev{};
	args->Handled = true;
	if (!use_mouse) return;
	if (GUICapture)
	{
		ev.type = EV_GUI_Event;
		ev.data1 = args->CurrentPoint->Position.X;
		ev.data2 = args->CurrentPoint->Position.Y;
		screen->ScaleCoordsFromWindow(ev.data1, ev.data2);
		switch (args->CurrentPoint->Properties->PointerUpdateKind)
		{
		case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
			ev.subtype = EV_GUI_LButtonUp;
			break;
		case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
			ev.subtype = EV_GUI_MButtonUp;
			break;
		case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
			ev.subtype = EV_GUI_RButtonUp;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton1Released:
			ev.subtype = EV_GUI_BackButtonUp;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton2Released:
			ev.subtype = EV_GUI_FwdButtonUp;
			break;
		}
		ev.data3 = (sender->GetKeyState(VirtualKey::Menu) == CoreVirtualKeyStates::Down ? GKM_ALT : 0)
			| (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down ? GKM_SHIFT : 0)
			| (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down ? GKM_CTRL : 0);
		game_events.push_back(ev);
	}
	else
	{
		ev.type = EV_KeyUp;
		switch (args->CurrentPoint->Properties->PointerUpdateKind)
		{
		case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
			ev.data1 = KEY_MOUSE1;
			break;
		case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
			ev.data1 = KEY_MOUSE3;
			break;
		case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
			ev.data1 = KEY_MOUSE2;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton1Released:
			ev.data1 = KEY_MOUSE4;
			break;
		case Windows::UI::Input::PointerUpdateKind::XButton2Released:
			ev.data1 = KEY_MOUSE5;
			break;
		}
		game_events.push_back(ev);
	}
}

void GameApp::OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	event_t ev{};
	args->Handled = true;
	if (!use_mouse) return;
	if (GUICapture)
	{
		ev.type = EV_GUI_Event;
		int WheelDelta = args->CurrentPoint->Properties->MouseWheelDelta;
		if (WheelDelta < 0)
		{
			ev.subtype = args->CurrentPoint->Properties->IsHorizontalMouseWheel ? EV_GUI_WheelLeft : EV_GUI_WheelDown;
			while (WheelDelta <= -WHEEL_DELTA)
			{
				game_events.push_back(ev);
				WheelDelta += WHEEL_DELTA;
			}
		}
		else
		{
			ev.subtype = args->CurrentPoint->Properties->IsHorizontalMouseWheel ? EV_GUI_WheelRight : EV_GUI_WheelUp;
			while (WheelDelta >= WHEEL_DELTA)
			{
				game_events.push_back(ev);
				WheelDelta -= WHEEL_DELTA;
			}
		}
	}
	else
	{
		ev.type = EV_KeyDown;
		int WheelDelta = args->CurrentPoint->Properties->MouseWheelDelta;
		if (WheelDelta < 0)
		{
			ev.data1 = args->CurrentPoint->Properties->IsHorizontalMouseWheel ? KEY_MWHEELLEFT : KEY_MWHEELDOWN;
		}
		else
		{
			ev.data1 = args->CurrentPoint->Properties->IsHorizontalMouseWheel ? KEY_MWHEELRIGHT : KEY_MWHEELUP;
		}
		game_events.push_back(ev);
		ev.type = EV_KeyUp;
		game_events.push_back(ev);
	}
}

void GameApp::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	OnKey(sender, args, false);
}

void GameApp::OnKey(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args, bool down)
{
	event_t ev{};
	args->Handled = true;
	if (GUICapture)
	{
		ev.type = EV_GUI_Event;
		ev.subtype = !down ? EV_GUI_KeyUp : ((args->KeyStatus.WasKeyDown) ? EV_GUI_KeyRepeat : EV_GUI_KeyDown);
		ev.data3 = ((args->KeyStatus.IsMenuKeyDown) ? GKM_ALT : 0)
			| (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down ? GKM_SHIFT : 0)
			| (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down ? GKM_CTRL : 0);
		switch (args->VirtualKey)
		{
		case VirtualKey::PageUp:			ev.data1 = GK_PGUP;			break;
		case VirtualKey::PageDown:			ev.data1 = GK_PGDN;			break;
		case VirtualKey::End:				ev.data1 = GK_END;			break;
		case VirtualKey::Home:				ev.data1 = GK_HOME;			break;
		case VirtualKey::Left:				ev.data1 = GK_LEFT;			break;
		case VirtualKey::Right:				ev.data1 = GK_RIGHT;		break;
		case VirtualKey::Up:				ev.data1 = GK_UP;			break;
		case VirtualKey::Down:				ev.data1 = GK_DOWN;			break;
		case VirtualKey::Delete:			ev.data1 = GK_DEL;			break;
		case VirtualKey::Escape:			ev.data1 = GK_ESCAPE;		break;
		case VirtualKey::F1:				ev.data1 = GK_F1;			break;
		case VirtualKey::F2:				ev.data1 = GK_F2;			break;
		case VirtualKey::F3:				ev.data1 = GK_F3;			break;
		case VirtualKey::F4:				ev.data1 = GK_F4;			break;
		case VirtualKey::F5:				ev.data1 = GK_F5;			break;
		case VirtualKey::F6:				ev.data1 = GK_F6;			break;
		case VirtualKey::F7:				ev.data1 = GK_F7;			break;
		case VirtualKey::F8:				ev.data1 = GK_F8;			break;
		case VirtualKey::F9:				ev.data1 = GK_F9;			break;
		case VirtualKey::F10:				ev.data1 = GK_F10;			break;
		case VirtualKey::F11:				ev.data1 = GK_F11;			break;
		case VirtualKey::F12:				ev.data1 = GK_F12;			break;
		case VirtualKey::GoBack:			ev.data1 = GK_BACK;			break;
		case VirtualKey::Enter:				ev.data1 = GK_RETURN;		break;
		case VirtualKey::Back:				ev.data1 = GK_BACKSPACE;	break;
		case VirtualKey::Tab:				ev.data1 = GK_TAB;			break;
		case VirtualKey::Add:				ev.data1 = '+';				break;
		case VirtualKey::Subtract:			ev.data1 = '-';				break;
		case VirtualKey::Divide:			ev.data1 = '/';				break;
		case VirtualKey::Multiply:			ev.data1 = '*';				break;
		case VirtualKey::Decimal:			ev.data1 = '.';				break;
		default:
		{
			if (args->VirtualKey >= VirtualKey::A && args->VirtualKey <= VirtualKey::Z)
			{
				ev.data1 = 'A' + (int)(args->VirtualKey - VirtualKey::A);
				break;
			}
			if (args->VirtualKey >= VirtualKey::Number0 && args->VirtualKey <= VirtualKey::Number9)
			{
				ev.data1 = '0' + (int)(args->VirtualKey - VirtualKey::Number0);
				break;
			}
			if (args->VirtualKey >= VirtualKey::NumberPad0 && args->VirtualKey <= VirtualKey::NumberPad9)
			{
				ev.data1 = '0' + (int)(args->VirtualKey - VirtualKey::NumberPad0);
				break;
			}
		}
		}
		game_events.push_back(ev);
		return;
	}
	else if (!down || (down && !(args->KeyStatus.WasKeyDown)))
	{
		FString keyString;
		keyString.Format("downkey: %d, repeat: %d, prevkeydown: %d\n", args->KeyStatus.ScanCode, args->KeyStatus.RepeatCount, args->KeyStatus.WasKeyDown);
		OutputDebugStringW(keyString.WideString().c_str());
		ev.type = down ? EV_KeyDown: EV_KeyUp;
		ev.data1 = args->KeyStatus.ScanCode;
		ev.data2 = Convert[ev.data1 & 0xFF];
		ev.data3 = (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down);
		if (args->KeyStatus.IsExtendedKey) ev.data1 |= 0x80;
		game_events.push_back(ev);
	}
}

void GameApp::OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args)
{
	event_t ev{};
	args->Handled = true;
	if (GUICapture && args->KeyCode <= 0xFFFF)
	{
		FString key;
		key.Format("Character 0x%lX\n", args->KeyCode);
		OutputDebugStringW(key.WideString().c_str());
		if (args->KeyCode == '\b' || args->KeyCode == '\t' || args->KeyCode == '\r' || args->KeyCode == '\n' || args->KeyCode == 27 || args->KeyCode == 4) return;
		if (args->KeyCode == '`')
		{
			ev.type = EV_GUI_Event;
			ev.subtype = EV_GUI_KeyDown;
			ev.data1 = (int16_t)args->KeyCode;
			ev.data3 = ((args->KeyStatus.IsMenuKeyDown) ? GKM_ALT : 0)
				| (sender->GetKeyState(VirtualKey::Shift) == CoreVirtualKeyStates::Down ? GKM_SHIFT : 0)
				| (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down ? GKM_CTRL : 0);
			game_events.push_back(ev);
			ev.subtype = EV_GUI_KeyUp;
			game_events.push_back(ev);
			ev.data2 = 0;
		}
		if (sender->GetKeyState(VirtualKey::Control) == CoreVirtualKeyStates::Down) return;
		ev.type = EV_GUI_Event;
		ev.subtype = EV_GUI_Char;
		ev.data1 = (int16_t)args->KeyCode;
		ev.data2 |= !!(args->KeyStatus.IsMenuKeyDown);
		game_events.push_back(ev);
	}
}

void GameApp::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	OnKey(sender, args, true);
}

void I_StartTic()
{
	GUICapture = sysCallbacks.WantGuiCapture();
	Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
	if (exitevent) throw CExitEvent(0);
	for (int i = 0; i < game_events.size(); i++)
	{
		D_PostEvent(&game_events[i]);
	}
	game_events.clear();
	bool captureModeInGame = sysCallbacks.CaptureModeInGame && sysCallbacks.CaptureModeInGame();
	bool wantNative = (!use_mouse || GUICapture || !captureModeInGame);

	if (!wantNative && sysCallbacks.WantNativeMouse && sysCallbacks.WantNativeMouse())
		wantNative = true;
	if (wantNative != nativeMouse)
	{
		nativeMouse = wantNative;
		if (wantNative)
			I_ReleaseMouseCapture();
		else
			I_SetMouseCapture();
	}
}
void I_StartFrame() {}
void I_GetAxes(float axes[NUM_JOYAXIS])
{
	for (int i = 0; i < NUM_JOYAXIS; i++)
	{
		axes[i] = 0;
	}
}

void I_ShutdownInput() {}
struct IJoystickConfig;
struct IJoystickConfig* I_UpdateDeviceList() { return nullptr; }
void I_GetJoysticks(TArray<struct IJoystickConfig*, struct IJoystickConfig*>&) {}