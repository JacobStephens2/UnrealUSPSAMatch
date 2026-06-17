#include "ShooterTarget.h"
#include "ShooterGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AShooterTarget::AShooterTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(1.0f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	RootComponent = Mesh;
}

void AShooterTarget::BeginPlay()
{
	Super::BeginPlay();
	ArenaCenter = FVector(0.f, 0.f, 100.f);

	// Tint the target red so it stands out against the grey floor.
	if (UMaterialInterface* Base = Mesh->GetMaterial(0))
	{
		UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		if (Dyn)
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.1f, 0.1f));
		}
	}
}

void AShooterTarget::OnShot()
{
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->AddScore(1);
	}
	Relocate();
}

void AShooterTarget::Relocate()
{
	const float Range = 1200.f;
	const FVector NewLoc = ArenaCenter + FVector(
		FMath::FRandRange(-Range, Range),
		FMath::FRandRange(-Range, Range),
		FMath::FRandRange(0.f, 400.f));
	SetActorLocation(NewLoc);
}
