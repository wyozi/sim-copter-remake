// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * `-SimCopterLogMouse`: logs every mouse press/release (and the cursor once a second) with the OS
 * position, Slate's cursor position, the window and game-viewport geometry and the widget under the
 * cursor. For diagnosing clicks that do not land - Mac windowed fullscreen at a resolution other
 * than the display's. Off unless the flag is on the command line.
 */
namespace SimCopterInputDebug
{
	void RegisterIfRequested();
}
