#include "ShooterTarget.h"
#include "ShooterGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AShooterTarget::AShooterTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = Mesh;

	AZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AZoneMesh"));
	AZoneMesh->SetupAttachment(Mesh);
	AZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USoundBase> SteelSnd(TEXT("/Game/Audio/steel.steel"));
	if (SteelSnd.Succeeded()) { SteelSound = SteelSnd.Object; }
	static ConstructorHelpers::FObjectFinder<USoundBase> PaperSnd(TEXT("/Game/Audio/paper.paper"));
	if (PaperSnd.Succeeded()) { PaperSound = PaperSnd.Object; }
}

void AShooterTarget::BeginPlay()
{
	Super::BeginPlay();
	InitialRotation = GetActorRotation();
	ApplyVisuals();

	if (AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->RegisterTarget(this);
	}
}

void AShooterTarget::ApplyVisuals()
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	AZoneMesh->SetVisibility(false);

	switch (Type)
	{
	case ETargetType::Paper:
	{
		// Thin upright cardboard target (local X = thin depth, Y = width, Z = height).
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetWorldScale3D(FVector(0.1f, 0.55f, 0.85f));
		HalfWidth = 0.55f * 50.f;
		HalfHeight = 0.85f * 50.f;
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Paper.M_Paper")))
		{
			Mesh->SetMaterial(0, M);
		}

		// Dark A-zone patch on the front face (local +X side, facing the shooter).
		AZoneMesh->SetStaticMesh(CubeMesh);
		AZoneMesh->SetVisibility(true);
		AZoneMesh->SetWorldScale3D(FVector(0.12f, 0.55f * 0.45f, 0.85f * 0.5f));
		AZoneMesh->SetRelativeLocation(FVector(6.f, 0.f, 0.f));
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_AZone.M_AZone")))
		{
			AZoneMesh->SetMaterial(0, M);
		}
		break;
	}
	case ETargetType::NoShoot:
	{
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetWorldScale3D(FVector(0.1f, 0.55f, 0.85f));
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_NoShoot.M_NoShoot")))
		{
			Mesh->SetMaterial(0, M);
		}
		break;
	}
	case ETargetType::Steel:
	{
		// Round popper on a stand.
		Mesh->SetStaticMesh(CylMesh);
		Mesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.95f));
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Steel.M_Steel")))
		{
			Mesh->SetMaterial(0, M);
		}
		break;
	}
	}
}

void AShooterTarget::HandleHit(const FHitResult& Hit)
{
	AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || GM->GetState() != EStageState::Running)
	{
		return; // only score on the clock
	}

	switch (Type)
	{
	case ETargetType::Paper:
		HitValues.Add(ZoneValueForHit(Hit));
		SpawnHitMarker(Hit.ImpactPoint);
		if (HitValues.Num() == RequiredHits) { MarkNeutralized(); }
		if (PaperSound) { UGameplayStatics::PlaySoundAtLocation(this, PaperSound, Hit.ImpactPoint); }
		GM->OnScoringHit();
		break;

	case ETargetType::NoShoot:
		++NoShootHits;
		SpawnHitMarker(Hit.ImpactPoint);
		if (PaperSound) { UGameplayStatics::PlaySoundAtLocation(this, PaperSound, Hit.ImpactPoint); }
		GM->OnScoringHit();
		break;

	case ETargetType::Steel:
		if (!bDown)
		{
			bDown = true;
			if (SteelSound) { UGameplayStatics::PlaySoundAtLocation(this, SteelSound, GetActorLocation()); }
			KnockDown();
			GM->OnScoringHit();
		}
		break;
	}
}

int32 AShooterTarget::ZoneValueForHit(const FHitResult& Hit) const
{
	// Hit point in the target's local frame; Y is across, Z is up.
	const FVector Local = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
	const float FY = FMath::Abs(Local.Y) / FMath::Max(HalfWidth, 1.f);
	const float FZ = FMath::Abs(Local.Z) / FMath::Max(HalfHeight, 1.f);

	if (FY < 0.45f && FZ < 0.5f)
	{
		return 5; // A
	}
	if (FY < 0.9f && FZ < 0.95f)
	{
		return 3; // C
	}
	return 1;     // D
}

void AShooterTarget::SpawnHitMarker(const FVector& WorldImpact)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) { return; }

	UStaticMeshComponent* Hole = NewObject<UStaticMeshComponent>(this);
	if (!Hole) { return; }
	Hole->SetStaticMesh(Cube);
	Hole->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hole->RegisterComponent();
	Hole->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform);

	// Sit just proud of the front face (local +X faces the shooter), aligned to the target.
	const FVector Front = GetActorForwardVector();
	Hole->SetWorldRotation(GetActorRotation());
	Hole->SetWorldLocation(WorldImpact + Front * 0.6f);
	Hole->SetWorldScale3D(FVector(0.06f, 0.075f, 0.075f)); // thin ~7cm disc

	// Dark hole, clearly visible on tan cardboard (reuse the A-zone material).
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_AZone.M_AZone")))
	{
		Hole->SetMaterial(0, M);
	}
	HitMarkers.Add(Hole);
}

void AShooterTarget::MarkNeutralized()
{
	// Green tint = "this paper target has its two hits" — readable across the bay.
	if (UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.62f, 0.24f));
	}
}

void AShooterTarget::KnockDown()
{
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Tip the popper over backwards.
	SetActorRotation(InitialRotation + FRotator(80.f, 0.f, 0.f));
}

void AShooterTarget::ResetTarget()
{
	HitValues.Empty();
	NoShootHits = 0;
	bDown = false;
	SetActorRotation(InitialRotation);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Clear bullet holes and restore the original (un-tinted) materials.
	for (UStaticMeshComponent* Marker : HitMarkers)
	{
		if (Marker) { Marker->DestroyComponent(); }
	}
	HitMarkers.Empty();
	ApplyVisuals();
}

int32 AShooterTarget::BestTwoSum() const
{
	TArray<int32> Sorted = HitValues;
	Sorted.Sort([](const int32& A, const int32& B) { return A > B; });
	int32 Sum = 0;
	for (int32 i = 0; i < Sorted.Num() && i < 2; ++i)
	{
		Sum += Sorted[i];
	}
	return Sum;
}

void AShooterTarget::CountBestTwoZones(int32& OutA, int32& OutC, int32& OutD) const
{
	TArray<int32> Sorted = HitValues;
	Sorted.Sort([](const int32& A, const int32& B) { return A > B; });
	for (int32 i = 0; i < Sorted.Num() && i < 2; ++i)
	{
		if (Sorted[i] == 5) ++OutA;
		else if (Sorted[i] == 3) ++OutC;
		else ++OutD;
	}
}
