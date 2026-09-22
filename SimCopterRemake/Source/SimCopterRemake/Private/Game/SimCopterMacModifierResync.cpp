// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/SimCopterMacModifierResync.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"

#if PLATFORM_MAC
// Not <AppKit/AppKit.h> directly: Carbon's NumberFormatting.h declares a struct FVector that
// collides with UE's. The engine's wrapper renames it around the system includes.
#include "Mac/MacSystemIncludes.h"
#endif

void SimCopterMacModifierResync::Tick(FSlateApplication& SlateApp)
{
#if PLATFORM_MAC
	// The posted event reaches the engine a frame later; do not queue one per frame meanwhile.
	static double LastPostSeconds = 0.0;
	const double NowSeconds = FPlatformTime::Seconds();
	if (NowSeconds - LastPostSeconds < 0.25)
	{
		return;
	}

	struct FModifierFamily
	{
		NSEventModifierFlags HardwareMask; // device-independent: any key of the family is down
		bool bLeftDown;
		NSUInteger LeftDeviceBit;          // the device-dependent bits FMacApplication reads
		bool bRightDown;
		NSUInteger RightDeviceBit;
	};

	// FMacApplication::GetModifierKeys reports the physical Control key as Command and vice versa
	// (so Cmd+C reads as Ctrl+C); the device bits are the ones its ConditionallyUpdateModifierKeys
	// tests.
	const FModifierKeysState State = SlateApp.GetModifierKeys();
	const FModifierFamily Families[] =
	{
		{ NSEventModifierFlagControl, State.IsLeftCommandDown(), 1u << 0, State.IsRightCommandDown(), 1u << 13 },
		{ NSEventModifierFlagCommand, State.IsLeftControlDown(), 1u << 3, State.IsRightControlDown(), 1u << 4 },
		{ NSEventModifierFlagShift, State.IsLeftShiftDown(), 1u << 1, State.IsRightShiftDown(), 1u << 2 },
		{ NSEventModifierFlagOption, State.IsLeftAltDown(), 1u << 5, State.IsRightAltDown(), 1u << 6 },
	};

	const NSEventModifierFlags Hardware = [NSEvent modifierFlags];
	NSUInteger Resynced = Hardware & NSEventModifierFlagDeviceIndependentFlagsMask;
	bool bAnyStale = false;
	for (const FModifierFamily& Family : Families)
	{
		if ((Hardware & Family.HardwareMask) == 0)
		{
			bAnyStale |= Family.bLeftDown || Family.bRightDown;
		}
		else
		{
			// Still physically held: keep exactly the sides the engine already knows about, so the
			// resync changes nothing but the stale families.
			Resynced |= (Family.bLeftDown ? Family.LeftDeviceBit : 0) | (Family.bRightDown ? Family.RightDeviceBit : 0);
		}
	}

	if (!bAnyStale)
	{
		return;
	}

	LastPostSeconds = NowSeconds;
	NSEvent* FlagsEvent = [NSEvent keyEventWithType:NSEventTypeFlagsChanged
										   location:NSZeroPoint
									  modifierFlags:Resynced
										  timestamp:[[NSProcessInfo processInfo] systemUptime]
									   windowNumber:0
											context:nil
										 characters:@""
						charactersIgnoringModifiers:@""
										  isARepeat:NO
											keyCode:0];
	// postEvent:atStart: is documented as safe to call off the main thread, which the game thread is.
	[NSApp postEvent:FlagsEvent atStart:NO];
#endif
}
