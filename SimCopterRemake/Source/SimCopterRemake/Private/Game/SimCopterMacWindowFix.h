// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Works around an engine bug that drops every mouse click in Mac windowed fullscreen.
 *
 * FMacWindow::GetFullScreenInfo (UE 5.8 MacWindow.cpp:243) converts the screen's TOP edge as
 * `origin.y - height + 1` where Cocoa's y grows upward, so the window's Slate position comes back a
 * whole screen-height below the display (y = 1799 on a 1440x900 screen). SWindow caches that, so
 * LocateWindowUnderMouse finds no window under the cursor and Slate discards the clicks; the keyboard
 * still works. It lasted until an unrelated macOS move event (a Cmd+Tab, say) re-cached the real
 * position, which is why it came and went.
 *
 * While the game window is in windowed fullscreen, a ticker moves it back onto the monitor it covers
 * whenever the cached position disagrees. A no-op everywhere but macOS.
 */
namespace SimCopterMacWindowFix
{
	void Register();
	void Unregister();
}
