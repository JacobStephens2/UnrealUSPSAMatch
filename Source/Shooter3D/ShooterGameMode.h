#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class AShooterTarget;

UENUM()
enum class EStageState : uint8
{
	PreStart,  // waiting for the shooter to start
	Standby,   // "standby" — random delay before the buzzer
	Running,   // on the clock
	Complete   // scored
};

UCLASS()
class SHOOTER3D_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShooterGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void RegisterTarget(AShooterTarget* Target);

	/** A scoring hit landed — refresh the live score and check for stage completion. */
	void OnScoringHit();

	/** Buzzer: begins the standby delay then the run. Also used to re-run when Complete. */
	void StartStage();

	// --- HUD queries ---
	EStageState GetState() const { return State; }
	float GetTime() const { return StageTime; }
	int32 GetPoints() const { return DisplayPoints; }
	float GetHitFactor() const { return HitFactor; }
	bool ShowGo() const { return bShowGo; }
	FString GetStatusText() const;
	FString GetResultText() const { return ResultText; }

protected:
	void BuildArena();
	void Beep();
	void ClearGo();
	void CompleteStage();

	/** Tallies points; when bFinal, also charges misses for un-engaged targets. */
	int32 ComputeScore(bool bFinal, int32& OutA, int32& OutC, int32& OutD, int32& OutMiss, int32& OutNoShoot) const;

	EStageState State = EStageState::PreStart;
	float StageTime = 0.f;
	int32 DisplayPoints = 0;
	float HitFactor = 0.f;
	bool bShowGo = false;
	FString ResultText;

	UPROPERTY()
	TArray<AShooterTarget*> Targets;

	UPROPERTY()
	class USoundBase* BuzzerSound = nullptr;

	UPROPERTY()
	class USoundBase* MusicSound = nullptr;

	FTimerHandle StandbyTimer;
	FTimerHandle GoTimer;
};
