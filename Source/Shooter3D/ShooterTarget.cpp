#include "ShooterTarget.h"
#include "ShooterGameMode.h"
#include "ShooterProjectile.h"
#include "ShooterCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AShooterTarget::AShooterTarget()
{
	PrimaryActorTick.bCanEverTick = true; // only the enemy actually ticks

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
	HomeLocation = GetActorLocation();
	ApplyVisuals();

	if (AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->RegisterTarget(this);
	}
}

void AShooterTarget::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Type == ETargetType::Enemy && !bDown)
	{
		TickEnemy(DeltaSeconds);
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
	case ETargetType::Enemy:
	{
		BuildRanchero();
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

	case ETargetType::Enemy:
		if (!bDown)
		{
			SpawnHitMarker(Hit.ImpactPoint);
			if (SteelSound) { UGameplayStatics::PlaySoundAtLocation(this, SteelSound, Hit.ImpactPoint); }
			if (--EnemyHP <= 0)
			{
				bDown = true;
				// Drop the ranchero over backwards.
				SetActorRotation(InitialRotation + FRotator(0.f, 0.f, 85.f));
				GM->OnScoringHit();
			}
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

void AShooterTarget::TintPart(UStaticMeshComponent* Part, const FLinearColor& Color)
{
	if (!Part) { return; }
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Paper.M_Paper")))
	{
		Part->SetMaterial(0, Base);
	}
	if (UMaterialInstanceDynamic* Dyn = Part->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AShooterTarget::BuildRanchero()
{
	for (UStaticMeshComponent* P : Parts) { if (P) { P->DestroyComponent(); } }
	Parts.Empty();

	UStaticMesh* Cube   = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cyl    = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	AZoneMesh->SetVisibility(false);

	// Root mesh = invisible 1 m collision box (centre-mass hitbox). Uniform scale so
	// the attached visual parts are not distorted.
	if (Cube) { Mesh->SetStaticMesh(Cube); }
	Mesh->SetWorldScale3D(FVector(1.f, 1.f, 1.f));
	Mesh->SetVisibility(false);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	const FLinearColor Poncho(0.58f, 0.16f, 0.13f); // serape red
	const FLinearColor Skin(0.80f, 0.62f, 0.46f);
	const FLinearColor Hat(0.30f, 0.19f, 0.10f);    // brown felt
	const FLinearColor Pants(0.18f, 0.13f, 0.09f);

	auto MakePart = [&](UStaticMesh* M, const FVector& Scale, const FVector& Loc, const FLinearColor& Col)
	{
		if (!M) { return; }
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
		P->SetStaticMesh(M);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->RegisterComponent();
		P->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform);
		P->SetRelativeLocation(Loc);
		P->SetRelativeScale3D(Scale);
		TintPart(P, Col);
		Parts.Add(P);
	};

	// Legs, arms, poncho torso, head, then the wide sombrero (brim + crown).
	MakePart(Cube,   FVector(0.22f, 0.26f, 0.55f), FVector(0.f, -15.f, -75.f), Pants);
	MakePart(Cube,   FVector(0.22f, 0.26f, 0.55f), FVector(0.f,  15.f, -75.f), Pants);
	MakePart(Cube,   FVector(0.18f, 0.18f, 0.62f), FVector(0.f, -40.f,  -5.f), Poncho);
	MakePart(Cube,   FVector(0.18f, 0.18f, 0.62f), FVector(0.f,  40.f,  -5.f), Poncho);
	MakePart(Cube,   FVector(0.34f, 0.66f, 0.85f), FVector(0.f,   0.f, -10.f), Poncho);
	MakePart(Sphere, FVector(0.34f, 0.34f, 0.34f), FVector(0.f,   0.f,  50.f), Skin);
	MakePart(Cyl,    FVector(1.00f, 1.00f, 0.06f), FVector(0.f,   0.f,  68.f), Hat);   // sombrero brim
	MakePart(Cyl,    FVector(0.46f, 0.46f, 0.42f), FVector(0.f,   0.f,  82.f), Hat);   // crown
}

void AShooterTarget::TickEnemy(float DeltaSeconds)
{
	AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
	const bool bRunning = GM && GM->GetState() == EStageState::Running;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

	// Strafe left/right across the player's view around the spawn spot.
	EnemyTime += DeltaSeconds;
	FVector Loc = HomeLocation;
	Loc.Y += FMath::Sin(EnemyTime * 1.1f) * 170.f;
	SetActorLocation(Loc);

	// Face the player (yaw only).
	if (Player)
	{
		FVector To = Player->GetActorLocation() - GetActorLocation();
		To.Z = 0.f;
		if (!To.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
		}
	}

	if (!bRunning || !Player) { return; }

	// Shoot back on a loose cadence.
	FireCountdown -= DeltaSeconds;
	if (FireCountdown <= 0.f)
	{
		FireCountdown = FMath::FRandRange(1.6f, 2.7f);
		if (UWorld* World = GetWorld())
		{
			const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, 40.f) + GetActorForwardVector() * 70.f;
			const FVector Aim = (Player->GetActorLocation() + FVector(0.f, 0.f, 30.f)) - Muzzle;
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AShooterProjectile* Shot = World->SpawnActor<AShooterProjectile>(
				AShooterProjectile::StaticClass(), Muzzle, Aim.Rotation(), Params))
			{
				Shot->ConfigureAsEnemyShot(15);
				Shot->FireInDirection(Aim);
			}
		}
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
	EnemyHP = 4;
	EnemyTime = 0.f;
	FireCountdown = 1.8f;
	SetActorRotation(InitialRotation);
	SetActorLocation(HomeLocation);
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
