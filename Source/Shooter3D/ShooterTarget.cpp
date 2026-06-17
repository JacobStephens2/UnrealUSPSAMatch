#include "ShooterTarget.h"
#include "ShooterGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
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

	// Red material (the engine's BasicShapeMaterial has no color slot, so we use
	// our own /Game/Materials/M_Target created with the material-authoring script).
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/Materials/M_Target.M_Target"));
	if (RedMat.Succeeded())
	{
		Mesh->SetMaterial(0, RedMat.Object);
	}

	RootComponent = Mesh;
}

void AShooterTarget::BeginPlay()
{
	Super::BeginPlay();
	ArenaCenter = FVector(0.f, 0.f, 100.f);
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
