#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterTarget.generated.h"

class UStaticMeshComponent;

UCLASS()
class SHOOTER3D_API AShooterTarget : public AActor
{
	GENERATED_BODY()

public:
	AShooterTarget();

	/** Called when a projectile strikes this target. Scores a point and respawns elsewhere. */
	void OnShot();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	/** Move to a fresh random spot inside the arena. */
	void Relocate();

	FVector ArenaCenter = FVector::ZeroVector;
};
