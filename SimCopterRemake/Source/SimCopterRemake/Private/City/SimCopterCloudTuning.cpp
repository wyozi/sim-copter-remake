// Copyright Epic Games, Inc. All Rights Reserved.

#include "City/SimCopterCloudTuning.h"

#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

namespace
{
float GViewSampleCountScale = 0.0f;
float GShadowViewSampleCountScale = 0.0f;

void ApplyToAllWorlds(IConsoleVariable*)
{
	for (TObjectIterator<UVolumetricCloudComponent> It; It; ++It)
	{
		SimCopterCloudTuning::Apply(It->GetWorld());
	}
}

FAutoConsoleVariableRef CVarViewSampleCountScale(
	TEXT("SimCopter.Clouds.ViewSampleCountScale"),
	GViewSampleCountScale,
	TEXT("Overrides the volumetric cloud layer's ViewSampleCountScale. <= 0 keeps the authored value."),
	FConsoleVariableDelegate::CreateStatic(&ApplyToAllWorlds),
	ECVF_Default);

FAutoConsoleVariableRef CVarShadowViewSampleCountScale(
	TEXT("SimCopter.Clouds.ShadowViewSampleCountScale"),
	GShadowViewSampleCountScale,
	TEXT("Overrides the volumetric cloud layer's ShadowViewSampleCountScale. <= 0 keeps the authored value."),
	FConsoleVariableDelegate::CreateStatic(&ApplyToAllWorlds),
	ECVF_Default);
}

void SimCopterCloudTuning::Apply(const UWorld* World)
{
	if (World == nullptr || (GViewSampleCountScale <= 0.0f && GShadowViewSampleCountScale <= 0.0f))
	{
		return;
	}

	for (TObjectIterator<UVolumetricCloudComponent> It; It; ++It)
	{
		UVolumetricCloudComponent* Cloud = *It;
		if (Cloud->GetWorld() != World)
		{
			continue;
		}
		// The setters, not the properties: they mark the render state dirty so the proxy updates.
		// Only on a difference, because this is re-run while the level plays (see the header).
		if (GViewSampleCountScale > 0.0f && !FMath::IsNearlyEqual(Cloud->ViewSampleCountScale, GViewSampleCountScale))
		{
			Cloud->SetViewSampleCountScale(GViewSampleCountScale);
		}
		if (GShadowViewSampleCountScale > 0.0f &&
			!FMath::IsNearlyEqual(Cloud->ShadowViewSampleCountScale, GShadowViewSampleCountScale))
		{
			Cloud->SetShadowViewSampleCountScale(GShadowViewSampleCountScale);
		}
	}
}
