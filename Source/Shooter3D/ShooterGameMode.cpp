#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "ShooterTarget.h"
#include "ShooterHUD.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShooterGameMode::AShooterGameMode()
{
	DefaultPawnClass = AShooterCharacter::StaticClass();
	HUDClass = AShooterHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<USoundBase> Buzz(TEXT("/Game/Audio/buzzer.buzzer"));
	if (Buzz.Succeeded()) { BuzzerSound = Buzz.Object; }
	static ConstructorHelpers::FObjectFinder<USoundBase> Mus(TEXT("/Game/Audio/music.music"));
	if (Mus.Succeeded()) { MusicSound = Mus.Object; }
}

void AShooterGameMode::StartPlay()
{
	Super::StartPlay();
	BuildArena();

	if (MusicSound)
	{
		UGameplayStatics::PlaySound2D(this, MusicSound, 0.4f); // looping bed at low volume
	}
}

void AShooterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State == EStageState::Running)
	{
		StageTime += DeltaSeconds;
		int32 A, C, D, Miss, NS;
		DisplayPoints = ComputeScore(false, A, C, D, Miss, NS);
	}
}

void AShooterGameMode::RegisterTarget(AShooterTarget* Target)
{
	if (Target)
	{
		Targets.AddUnique(Target);
	}
}

// ---------------------------------------------------------------------------
// Stage flow: PreStart -> (START) -> Standby -> (buzzer) -> Running -> Complete
// ---------------------------------------------------------------------------

void AShooterGameMode::StartStage()
{
	if (State == EStageState::Standby || State == EStageState::Running)
	{
		return;
	}

	if (State == EStageState::Complete)
	{
		if (bFailed)
		{
			BuildStage(CurrentStage);          // retry the same stage
		}
		else if (CurrentStage < NumStages - 1)
		{
			BuildStage(++CurrentStage);        // advance to the next stage
		}
		else
		{
			CurrentStage = 0;
			BuildStage(0);                     // new match
		}
		return; // now in PreStart for the (re)built stage — press again to run
	}

	BeginRun(); // PreStart -> run the current stage
}

void AShooterGameMode::BeginRun()
{
	for (AShooterTarget* T : Targets)
	{
		if (T) { T->ResetTarget(); }
	}

	StageTime = 0.f;
	DisplayPoints = 0;
	HitFactor = 0.f;
	PlayerHealth = 100;
	bFailed = false;
	ResultText.Reset();
	State = EStageState::Standby;

	// USPSA-style random start delay before the buzzer.
	const float Delay = FMath::FRandRange(1.0f, 2.5f);
	GetWorldTimerManager().SetTimer(StandbyTimer, this, &AShooterGameMode::Beep, Delay, false);
}

void AShooterGameMode::PlayerHit(int32 Damage)
{
	if (State != EStageState::Running) { return; }
	PlayerHealth -= Damage;
	if (PlayerHealth <= 0)
	{
		PlayerHealth = 0;
		FailStage();
	}
}

void AShooterGameMode::FailStage()
{
	State = EStageState::Complete;
	bFailed = true;
	ResultText = FString::Printf(TEXT("YOU'RE HIT — STAGE FAILED\nTime  %.2fs"), StageTime);
}

void AShooterGameMode::Beep()
{
	State = EStageState::Running;
	StageTime = 0.f;
	bShowGo = true;
	if (BuzzerSound)
	{
		UGameplayStatics::PlaySound2D(this, BuzzerSound);
	}
	GetWorldTimerManager().SetTimer(GoTimer, this, &AShooterGameMode::ClearGo, 0.7f, false);
}

void AShooterGameMode::ClearGo()
{
	bShowGo = false;
}

void AShooterGameMode::OnScoringHit()
{
	int32 A, C, D, Miss, NS;
	DisplayPoints = ComputeScore(false, A, C, D, Miss, NS);

	// Stage is done when every paper has its two hits, every popper is down,
	// and the ranchero (if any) is on the ground.
	bool bDone = true;
	for (AShooterTarget* T : Targets)
	{
		if (!T) { continue; }
		if (T->GetTargetType() == ETargetType::Paper && T->NumPaperHits() < 2) { bDone = false; break; }
		if (T->GetTargetType() == ETargetType::Steel && !T->IsSteelDown()) { bDone = false; break; }
		if (T->GetTargetType() == ETargetType::Enemy && !T->IsEnemyDown()) { bDone = false; break; }
	}
	if (bDone)
	{
		CompleteStage();
	}
}

void AShooterGameMode::CompleteStage()
{
	State = EStageState::Complete;

	int32 A = 0, C = 0, D = 0, Miss = 0, NS = 0;
	const int32 Pts = ComputeScore(true, A, C, D, Miss, NS);
	DisplayPoints = Pts;
	HitFactor = FMath::Max(0, Pts) / FMath::Max(StageTime, 0.01f);

	const bool bFinal = (CurrentStage == NumStages - 1);
	const FString Header = bFinal
		? TEXT("MATCH COMPLETE")
		: FString::Printf(TEXT("STAGE %d of %d CLEAR"), CurrentStage + 1, NumStages);

	ResultText = FString::Printf(
		TEXT("%s\nTime  %.2fs\nA %d   C %d   D %d\nMisses %d   No-shoots %d\nPoints  %d\nHIT FACTOR  %.2f"),
		*Header, StageTime, A, C, D, Miss, NS, Pts, HitFactor);
}

int32 AShooterGameMode::ComputeScore(bool bFinal, int32& OutA, int32& OutC, int32& OutD, int32& OutMiss, int32& OutNoShoot) const
{
	OutA = OutC = OutD = OutMiss = OutNoShoot = 0;
	int32 Pts = 0;

	for (AShooterTarget* T : Targets)
	{
		if (!T) { continue; }
		switch (T->GetTargetType())
		{
		case ETargetType::Paper:
			Pts += T->BestTwoSum();
			T->CountBestTwoZones(OutA, OutC, OutD);
			if (bFinal)
			{
				OutMiss += FMath::Max(0, 2 - T->NumPaperHits());
			}
			break;

		case ETargetType::Steel:
			if (T->IsSteelDown()) { Pts += 5; }
			else if (bFinal) { ++OutMiss; }
			break;

		case ETargetType::NoShoot:
			OutNoShoot += T->GetNoShootHits();
			break;

		case ETargetType::Enemy:
			if (T->IsEnemyDown()) { Pts += 15; }
			break;
		}
	}

	Pts -= OutNoShoot * 10; // no-shoot penalty
	Pts -= OutMiss * 10;    // miss penalty
	return Pts;
}

FString AShooterGameMode::GetStatusText() const
{
	switch (State)
	{
	case EStageState::PreStart:
		if (StageHasEnemy())
		{
			return FString::Printf(TEXT("STAGE %d of %d — THE OUTLAW\nA ranchero bandit moves and shoots back!\nPut him down, clear the paper & steel."),
				CurrentStage + 1, NumStages);
		}
		return FString::Printf(TEXT("STAGE %d of %d — SPEED BAY\nEngage every target: best 2 hits on paper,\nknock the steel, avoid the white no-shoots."),
			CurrentStage + 1, NumStages);
	case EStageState::Standby:
		return TEXT("STANDBY...");
	case EStageState::Running:
		return FString();
	case EStageState::Complete:
		return FString();
	}
	return FString();
}

FString AShooterGameMode::GetButtonLabel() const
{
	if (State != EStageState::Complete)
	{
		return TEXT("START");
	}
	if (bFailed)
	{
		return TEXT("RETRY STAGE");
	}
	return (CurrentStage < NumStages - 1) ? TEXT("NEXT STAGE") : TEXT("NEW MATCH");
}

// ---------------------------------------------------------------------------
// Stage geometry — built at runtime (floor, lights, targets).
// ---------------------------------------------------------------------------

void AShooterGameMode::BuildArena()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Floor.
	if (Cube)
	{
		if (AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator))
		{
			Floor->SetMobility(EComponentMobility::Movable);
			UStaticMeshComponent* MC = Floor->GetStaticMeshComponent();
			MC->SetStaticMesh(Cube);
			MC->SetWorldScale3D(FVector(60.f, 60.f, 1.f));
			MC->SetWorldLocation(FVector(0.f, 0.f, -50.f));
			if (UMaterialInstanceDynamic* Dyn = MC->CreateAndSetMaterialInstanceDynamic(0))
			{
				Dyn->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.24f, 0.27f));
			}
		}
	}

	// Lights: warm key + cool fill + overhead fill (no SkyAtmosphere on mobile LDR).
	auto SpawnLight = [&](const FRotator& Rot, float Intensity, const FLinearColor& Color)
	{
		if (ADirectionalLight* L = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 2000.f), Rot))
		{
			L->SetMobility(EComponentMobility::Movable);
			if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(L->GetLightComponent()))
			{
				DLC->SetIntensity(Intensity);
				DLC->SetLightColor(Color);
			}
		}
	};
	SpawnLight(FRotator(-50.f, -60.f, 0.f), 10.f, FLinearColor(1.0f, 0.97f, 0.9f));
	SpawnLight(FRotator(-25.f, 120.f, 0.f), 5.f, FLinearColor(0.55f, 0.65f, 1.0f));
	SpawnLight(FRotator(-88.f, 0.f, 0.f), 3.f, FLinearColor::White);

	BuildStage(0);
}

void AShooterGameMode::BuildStage(int32 Index)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// Tear down the previous stage's targets.
	for (AShooterTarget* T : Targets)
	{
		if (T) { T->Destroy(); }
	}
	Targets.Reset();

	auto SpawnTarget = [&](ETargetType Ty, const FVector& Loc)
	{
		// Face the shooter column at (0,0).
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Loc.Y, -Loc.X));
		const FTransform Xf(FRotator(0.f, Yaw, 0.f), Loc);
		AShooterTarget* T = World->SpawnActorDeferred<AShooterTarget>(AShooterTarget::StaticClass(), Xf);
		if (T)
		{
			T->SetTargetType(Ty);
			T->FinishSpawning(Xf);
		}
	};

	if (Index == 0)
	{
		// Stage 1 — a static speed bay.
		SpawnTarget(ETargetType::Paper,   FVector(520.f, -360.f, 150.f));
		SpawnTarget(ETargetType::Paper,   FVector(560.f, -150.f, 150.f));
		SpawnTarget(ETargetType::NoShoot, FVector(600.f, 0.f, 150.f));
		SpawnTarget(ETargetType::Paper,   FVector(560.f, 160.f, 150.f));
		SpawnTarget(ETargetType::Paper,   FVector(540.f, 370.f, 150.f));
		SpawnTarget(ETargetType::Steel,   FVector(780.f, -250.f, 110.f));
		SpawnTarget(ETargetType::Steel,   FVector(800.f, 250.f, 110.f));
	}
	else
	{
		// Stage 2 — fewer paper, plus a moving ranchero that shoots back.
		SpawnTarget(ETargetType::Paper,   FVector(560.f, -420.f, 150.f));
		SpawnTarget(ETargetType::Paper,   FVector(620.f, -230.f, 150.f));
		SpawnTarget(ETargetType::NoShoot, FVector(640.f, 430.f, 150.f));
		SpawnTarget(ETargetType::Steel,   FVector(820.f, 300.f, 110.f));
		SpawnTarget(ETargetType::Enemy,   FVector(720.f, 80.f, 110.f));
	}

	// Fresh stage state.
	State = EStageState::PreStart;
	StageTime = 0.f;
	DisplayPoints = 0;
	HitFactor = 0.f;
	PlayerHealth = 100;
	bShowGo = false;
	bFailed = false;
	ResultText.Reset();
}
