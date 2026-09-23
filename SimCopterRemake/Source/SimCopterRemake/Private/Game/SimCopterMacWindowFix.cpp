// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/SimCopterMacWindowFix.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimCopterMacWindowFix, Log, All);

namespace
{
FTSTicker::FDelegateHandle GTickerHandle;

#if PLATFORM_MAC
bool Tick(float)
{
	if (GEngine == nullptr || GEngine->GameViewport == nullptr || !FSlateApplication::IsInitialized())
	{
		return true;
	}
	const TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
	if (!Window.IsValid() || !Window->IsVisible() || Window->GetWindowMode() != EWindowMode::WindowedFullscreen)
	{
		return true;
	}

	const FVector2D Position = Window->GetPositionInScreen();
	const FVector2D Size = Window->GetSizeInScreen();

	FDisplayMetrics Metrics;
	FSlateApplication::Get().GetCachedDisplayMetrics(Metrics);

	// The monitor this window fills: the bug leaves X right and only Y wrong, so match on X and
	// size, and fall back to the primary display.
	const FMonitorInfo* Target = nullptr;
	for (const FMonitorInfo& Monitor : Metrics.MonitorInfo)
	{
		const FPlatformRect& Rect = Monitor.DisplayRect;
		if (FMath::IsNearlyEqual(Position.X, static_cast<double>(Rect.Left), 1.0) &&
			FMath::IsNearlyEqual(Size.X, static_cast<double>(Rect.Right - Rect.Left), 1.0) &&
			FMath::IsNearlyEqual(Size.Y, static_cast<double>(Rect.Bottom - Rect.Top), 1.0))
		{
			Target = &Monitor;
			break;
		}
	}
	if (Target == nullptr)
	{
		for (const FMonitorInfo& Monitor : Metrics.MonitorInfo)
		{
			if (Monitor.bIsPrimary)
			{
				Target = &Monitor;
				break;
			}
		}
	}
	if (Target == nullptr)
	{
		return true;
	}

	const FVector2D Expected(Target->DisplayRect.Left, Target->DisplayRect.Top);
	if (!Position.Equals(Expected, 1.0))
	{
		UE_LOG(LogSimCopterMacWindowFix, Display,
			TEXT("Windowed fullscreen window cached at %s instead of %s; moving it back (engine GetFullScreenInfo flip)."),
			*Position.ToString(), *Expected.ToString());
		Window->MoveWindowTo(Expected);
	}
	return true;
}
#endif
}

void SimCopterMacWindowFix::Register()
{
#if PLATFORM_MAC
	if (!GTickerHandle.IsValid())
	{
		GTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
	}
#endif
}

void SimCopterMacWindowFix::Unregister()
{
	if (GTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(GTickerHandle);
		GTickerHandle.Reset();
	}
}
