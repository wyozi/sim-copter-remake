// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateApplication;

/**
 * Releases modifier keys that macOS took the key-up of. Remake-only; there is no original behaviour.
 *
 * Control is "collective down", and Control+arrow is Mission Control / switch Space on a stock Mac.
 * Hold Control to descend, steer with an arrow, and the window server keeps the chord - including
 * Control's release. FMacApplication only re-reads the modifier flags on mouse and flags-changed
 * events, never on key presses, so a keyboard-only pilot is left with Control held forever: the
 * collective axis sums Space (+1) and Control (-1) to zero and Space "stops working".
 *
 * Called every Slate tick. When the engine believes a modifier is down that the hardware says is
 * up, it posts one synthetic NSEventTypeFlagsChanged carrying the real state, and the engine's own
 * ConditionallyUpdateModifierKeys sends the key-up and fixes its cached flags. Faking the key-up
 * directly would leave that cache stale, and the next real press of the key would be swallowed.
 * A no-op everywhere but macOS.
 */
namespace SimCopterMacModifierResync
{
	void Tick(FSlateApplication& SlateApp);
}
