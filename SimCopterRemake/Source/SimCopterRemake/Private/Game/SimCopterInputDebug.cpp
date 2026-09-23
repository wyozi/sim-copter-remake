// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/SimCopterInputDebug.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimCopterInputDebug, Log, All);

namespace
{
class FSimCopterMouseLogger : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
		SecondsSinceMoveLog += DeltaTime;
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (SecondsSinceMoveLog >= 1.0f)
		{
			SecondsSinceMoveLog = 0.0f;
			Log(TEXT("move"), SlateApp, MouseEvent);
		}
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		Log(TEXT("DOWN"), SlateApp, MouseEvent);
		return false;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		Log(TEXT("up"), SlateApp, MouseEvent);
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("SimCopterMouseLogger"); }

private:
	void Log(const TCHAR* What, FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const
	{
		const FVector2D EventPos = MouseEvent.GetScreenSpacePosition();
		const FVector2D CursorPos = SlateApp.GetCursorPos();

		FString WindowText = TEXT("<no window>");
		FString WidgetText = TEXT("<nothing>");
		const FWidgetPath Path = SlateApp.LocateWindowUnderMouse(
			EventPos, SlateApp.GetInteractiveTopLevelWindows(), /*bIgnoreEnabledStatus=*/true);
		if (Path.IsValid())
		{
			const FGeometry& WindowGeometry = Path.Widgets[0].Geometry;
			WindowText = FString::Printf(TEXT("window pos %s size %s scale %.3f"),
				*WindowGeometry.GetAbsolutePosition().ToString(),
				*WindowGeometry.GetAbsoluteSize().ToString(),
				WindowGeometry.Scale);
			WidgetText = Path.Widgets.Last().Widget->GetTypeAsString();
		}

		FString ViewportText = TEXT("<no game viewport>");
		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			FVector2D ViewportSize = FVector2D::ZeroVector;
			GEngine->GameViewport->GetViewportSize(ViewportSize);
			ViewportText = FString::Printf(TEXT("viewport %s"), *ViewportSize.ToString());
		}

		UE_LOG(LogSimCopterInputDebug, Display, TEXT("MOUSE %s event %s cursor %s | %s | %s | under: %s"),
			What, *EventPos.ToString(), *CursorPos.ToString(), *WindowText, *ViewportText, *WidgetText);

		// Why LocateWindowUnderMouse can come back empty: every interactive top-level window, what
		// Slate caches for it, and whether the native window itself thinks the cursor is inside.
		const TArray<TSharedRef<SWindow>> Windows = SlateApp.GetInteractiveTopLevelWindows();
		for (const TSharedRef<SWindow>& Window : Windows)
		{
			const TSharedPtr<FGenericWindow> Native = Window->GetNativeWindow();
			const FVector2D LocalCursor = EventPos - Window->GetPositionInScreen();
			int32 NX = 0, NY = 0, NW = 0, NH = 0;
			const bool bHasRect = Native.IsValid() && Native->GetFullScreenInfo(NX, NY, NW, NH);
			UE_LOG(LogSimCopterInputDebug, Display,
				TEXT("MOUSE   window '%s' type %d slatePos %s slateSize %s visible %d active %d acceptsInput %d mode %d nativeInside %d fullscreenInfo %d:(%d,%d %dx%d)"),
				*Window->GetTitle().ToString(), static_cast<int32>(Window->GetType()),
				*Window->GetPositionInScreen().ToString(), *Window->GetSizeInScreen().ToString(),
				Window->IsVisible() ? 1 : 0, Window->IsActive() ? 1 : 0, Window->AcceptsInput() ? 1 : 0,
				static_cast<int32>(Window->GetWindowMode()),
				Native.IsValid() && Native->IsPointInWindow(FMath::RoundToInt(LocalCursor.X), FMath::RoundToInt(LocalCursor.Y)) ? 1 : 0,
				bHasRect ? 1 : 0, NX, NY, NW, NH);
		}
		if (Windows.Num() == 0)
		{
			UE_LOG(LogSimCopterInputDebug, Display, TEXT("MOUSE   no interactive top-level windows"));
		}
	}

	float SecondsSinceMoveLog = 1.0f;
};
}

void SimCopterInputDebug::RegisterIfRequested()
{
	static bool bRegistered = false;
	if (bRegistered || !FSlateApplication::IsInitialized() ||
		!FParse::Param(FCommandLine::Get(), TEXT("SimCopterLogMouse")))
	{
		return;
	}
	bRegistered = true;
	FSlateApplication::Get().RegisterInputPreProcessor(MakeShared<FSimCopterMouseLogger>());
	UE_LOG(LogSimCopterInputDebug, Display, TEXT("Mouse logging on (-SimCopterLogMouse)."));
}
