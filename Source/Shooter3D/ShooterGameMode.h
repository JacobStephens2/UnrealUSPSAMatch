#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

UCLASS()
class SHOOTER3D_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShooterGameMode();

	virtual void StartPlay() override;

	void AddScore(int32 Delta) { Score += Delta; }
	int32 GetScore() const { return Score; }

protected:
	int32 Score = 0;

	/** Build the play space at runtime: floor, lighting, sky, and a handful of targets. */
	void BuildArena();
};
