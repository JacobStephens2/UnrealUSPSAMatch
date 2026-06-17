#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "ShooterTarget.h"
#include "ShooterHUD.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AShooterGameMode::AShooterGameMode()
{
	DefaultPawnClass = AShooterCharacter::StaticClass();
	HUDClass = AShooterHUD::StaticClass();
}

void AShooterGameMode::StartPlay()
{
	Super::StartPlay();
	BuildArena();
}

void AShooterGameMode::BuildArena()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// NOTE: ConstructorHelpers::FObjectFinder is ONLY valid inside a constructor.
	// BuildArena() runs at StartPlay, so load the mesh at runtime with LoadObject.
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	// --- Floor: a big flat scaled cube ---
	if (Cube)
	{
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(
			FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator);
		if (Floor)
		{
			Floor->SetMobility(EComponentMobility::Movable);
			UStaticMeshComponent* MC = Floor->GetStaticMeshComponent();
			MC->SetStaticMesh(Cube);
			MC->SetWorldScale3D(FVector(60.f, 60.f, 1.f));
			MC->SetWorldLocation(FVector(0.f, 0.f, -50.f));
			if (UMaterialInstanceDynamic* Dyn = MC->CreateAndSetMaterialInstanceDynamic(0))
			{
				Dyn->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.27f, 0.3f));
			}
		}
	}

	// --- Lighting: two movable directional lights (key + fill).
	// Deliberately NO SkyAtmosphere / real-time-capture SkyLight: those are not
	// supported on the mobile LDR forward path (MobileHDR=False) and crash on device.
	if (ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(
		FVector(0.f, 0.f, 2000.f), FRotator(-50.f, -60.f, 0.f)))
	{
		Key->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(Key->GetLightComponent()))
		{
			DLC->SetIntensity(10.0f);
			DLC->SetLightColor(FLinearColor(1.0f, 0.97f, 0.9f));
		}
	}

	// Cool-toned fill from the opposite side so back faces aren't pure black.
	if (ADirectionalLight* Fill = World->SpawnActor<ADirectionalLight>(
		FVector(0.f, 0.f, 2000.f), FRotator(-25.f, 120.f, 0.f)))
	{
		Fill->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(Fill->GetLightComponent()))
		{
			DLC->SetIntensity(5.0f);
			DLC->SetLightColor(FLinearColor(0.55f, 0.65f, 1.0f));
		}
	}

	// Overhead fill so the floor/tops aren't flat-dark (cheap stand-in for skylight ambient).
	if (ADirectionalLight* Top = World->SpawnActor<ADirectionalLight>(
		FVector(0.f, 0.f, 2000.f), FRotator(-88.f, 0.f, 0.f)))
	{
		Top->SetMobility(EComponentMobility::Movable);
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(Top->GetLightComponent()))
		{
			DLC->SetIntensity(3.0f);
		}
	}

	// --- Targets arranged in a ring around the spawn point ---
	const int32 NumTargets = 8;
	const float Radius = 900.f;
	for (int32 i = 0; i < NumTargets; ++i)
	{
		const float Angle = (2.f * PI * i) / NumTargets;
		const FVector Loc(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 120.f);
		World->SpawnActor<AShooterTarget>(Loc, FRotator::ZeroRotator);
	}
}
