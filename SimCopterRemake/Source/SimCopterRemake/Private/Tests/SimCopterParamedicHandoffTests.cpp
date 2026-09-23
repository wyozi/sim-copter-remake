#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Flight/SimCopterHelicopterPawn.h"
#include "Formats/SimCity2000Reader.h"
#include "Ground/SimCopterGroundAgent.h"
#include "Ground/SimCopterTrafficSystemActor.h"
#include "Missions/SimCopterMissionSystemActor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimCopterParamedicCabinHandoffTest,
	"SimCopter.Missions.ParamedicCabinHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimCopterParamedicCabinHandoffTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &InitValues);
	ASimCopterTrafficSystemActor* Traffic = World->SpawnActor<ASimCopterTrafficSystemActor>();
	Traffic->PeopleTileClasses.Init(7, FSimCity2000City::TileCount);
	Traffic->ActiveTileSize = 400.0f;
	World->SpawnActor<ASimCopterMissionSystemActor>();
	ASimCopterGroundAgent* Medic = World->SpawnActor<ASimCopterGroundAgent>();
	ASimCopterGroundAgent* Patient = World->SpawnActor<ASimCopterGroundAgent>();
	ASimCopterHelicopterPawn* Helicopter = World->SpawnActor<ASimCopterHelicopterPawn>();
	Medic->SetOwner(Traffic);
	Patient->SetOwner(Traffic);
	Medic->BehaviorContext.Attributes[EBhavAttr::State] = 5;
	Patient->BehaviorContext.Attributes[EBhavAttr::State] = 6;
	// A hospital medic remains persistent after joining the player's crew, but boarding
	// relinquishes the roof post. Returning crew must not inherit the posted-worker gate.
	Medic->SetPersistentHospitalRoofCrew(true);
	Traffic->PedestrianAgents.Add(Patient);
	TestTrue(TEXT("Patient is carried by the medic"), Patient->BoardCarrier(Medic, false, false, true));
	ISimCopterBehaviorWorld& MedicActions = *Medic;
	FSimCopterPersonContext Context;
	TestFalse(TEXT("Missing destination refuses transfer"), MedicActions.SelectCarriedPerson(Context, false));
	TestTrue(TEXT("Missing destination keeps patient carried"), Patient->GetBehaviorCarrier() == Medic);
	Context.SelectedObject = Helicopter;
	Context.bHasSelection = true;
	const int32 SeatsBefore = Helicopter->GetAvailablePassengerSeats();
	TestTrue(TEXT("Test helicopter has room"), SeatsBefore > 0);
	TestTrue(TEXT("Opcode 46 handoff succeeds"), MedicActions.SelectCarriedPerson(Context, false));
	TestTrue(TEXT("Same patient now rides helicopter"), Patient->GetBehaviorCarrier() == Helicopter);
	TestTrue(TEXT("Patient is hidden inside cabin"), Patient->IsHidden());
	TestEqual(TEXT("Patient consumes one seat"), Helicopter->GetAvailablePassengerSeats(), SeatsBefore - 1);
	TestTrue(TEXT("Handoff selects the transferred patient"), Context.SelectedObject.Get() == Patient);
	TestNull(TEXT("Medic no longer totes patient"), Traffic->FindPersonCarriedBy(*Medic));
	TestTrue(TEXT("Returning medic selects helicopter after patient handoff"), MedicActions.SelectOwningVehicle(Context));
	TestTrue(TEXT("Return destination is helicopter"), Context.SelectedObject.Get() == Helicopter);
	TestTrue(TEXT("Returning medic boards with patient already inside"), MedicActions.BoardSelection(Context));
	TestTrue(TEXT("Medic and patient share the helicopter"), Medic->GetBehaviorCarrier() == Patient->GetBehaviorCarrier());
	TestEqual(TEXT("Medic and patient occupy separate seats"), Helicopter->GetAvailablePassengerSeats(), SeatsBefore - 2);
	Medic->AlightFromCarrier();

	Medic->SetHospitalRoofPost(Medic->GetActorLocation(), 200.0f);
	TestFalse(TEXT("Posted hospital worker cannot select ride instead of unloading"), MedicActions.SelectOwningVehicle(Context));
	TestFalse(TEXT("Posted hospital worker cannot bypass boarding gate"), Medic->BoardCarrier(Helicopter, false));
	Medic->SetPersistentHospitalRoofCrew(false);

	Patient->BoardCarrier(Medic, false, false, true);
	Helicopter->AddMissionPassengersForMission(Helicopter->GetAvailablePassengerSeats(), INDEX_NONE,
		Patient->GetMissionPassengerKind());
	Context.SelectedObject = Helicopter;
	TestFalse(TEXT("Full cabin refuses handoff"), MedicActions.SelectCarriedPerson(Context, false));
	TestTrue(TEXT("Full cabin leaves patient with medic"), Patient->GetBehaviorCarrier() == Medic);
	TestTrue(TEXT("Failed transfer retains vehicle selection"), Context.SelectedObject.Get() == Helicopter);
	World->DestroyWorld(false);
	return true;
}


// A medevac is delivered at ANY hospital (FUN_004a7a10's 0x20 branch gives it no +0x30), so the
// mission actor serves every hospital footprint the city has. It gets them one per building from
// the people scene's per-footprint nodes, keyed on the footprint origin that
// EnsureHospitalParamedicAtTile and the roof-post cache use.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimCopterHospitalSitesTest,
	"SimCopter.Missions.HospitalSites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimCopterHospitalSitesTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &InitValues);
	ASimCopterTrafficSystemActor* Traffic = World->SpawnActor<ASimCopterTrafficSystemActor>();

	TArray<ASimCopterTrafficSystemActor::FHospitalSite> Sites;
	Traffic->GetHospitalSites(Sites);
	TestEqual(TEXT("A city with no hospital has no sites"), Sites.Num(), 0);

	auto AddNode = [Traffic](const int32 X, const int32 Y, const uint8 BuildingId, const int32 Size, const FVector& Location)
	{
		FSimCopterGroundRouteNode& Node = Traffic->PedestrianNodes.AddDefaulted_GetRef();
		Node.FileX = X;
		Node.FileY = Y;
		Node.BuildingId = BuildingId;
		Node.PeopleFootprintSize = Size;
		Node.Location = Location;
	};
	AddNode(91, 62, 0xD1, 3, FVector(100, 200, 0));
	AddNode(10, 10, 0x80, 1, FVector::ZeroVector);
	AddNode(16, 76, 0xD1, 3, FVector(300, 400, 0));

	Traffic->GetHospitalSites(Sites);
	TestEqual(TEXT("One site per hospital building"), Sites.Num(), 2);
	if (Sites.Num() == 2)
	{
		TestEqual(TEXT("First hospital keyed on its footprint origin"), Sites[0].OriginTile, FIntPoint(91, 62));
		TestEqual(TEXT("First hospital centre is the footprint node"), Sites[0].Center, FVector(100, 200, 0));
		TestEqual(TEXT("Second hospital keyed on its footprint origin"), Sites[1].OriginTile, FIntPoint(16, 76));
	}
	World->DestroyWorld(false);
	return true;
}

#endif
