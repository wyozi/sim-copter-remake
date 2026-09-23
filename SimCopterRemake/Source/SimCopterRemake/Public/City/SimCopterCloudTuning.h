// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Runtime overrides for the level's volumetric cloud layer (CelestialVault's component, which lives
 * in an engine plugin and so is tuned here rather than in its asset). Driven by two CVars that
 * default to "leave the authored value":
 *
 *   SimCopter.Clouds.ViewSampleCountScale        (<= 0: authored, 1.0)
 *   SimCopter.Clouds.ShadowViewSampleCountScale  (<= 0: authored, 1.0)
 *
 * The Mac device profile sets both to 0.25. Measured on an M1 looking at the sky, that takes the
 * clouds from ~42 ms to ~3.6 ms and the screenshots are indistinguishable; the r.VolumetricCloud.*
 * max-sample CVars do nothing here because they only clamp, and the component asks for fewer.
 * See Docs/memory/mac-performance.md.
 */
namespace SimCopterCloudTuning
{
	// Applies the CVars to every volumetric cloud component in the world, touching a component only
	// when its value differs. Applying once at BeginPlay did not stick - something re-initialises
	// the cloud layer after the city actor starts - so the city actor re-runs this once a second;
	// the CVars also re-apply to every live world when they change.
	void Apply(const UWorld* World);
}
