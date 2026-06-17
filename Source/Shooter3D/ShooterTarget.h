#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterTarget.generated.h"

class UStaticMeshComponent;

UENUM()
enum class ETargetType : uint8
{
	Paper,    // scored A/C/D, best 2 hits count
	NoShoot,  // penalty if hit
	Steel     // knock-down popper, must fall
};

UCLASS()
class SHOOTER3D_API AShooterTarget : public AActor
{
	GENERATED_BODY()

public:
	AShooterTarget();

	/** Set before FinishSpawning so BeginPlay can build the right visuals. */
	void SetTargetType(ETargetType InType) { Type = InType; }
	ETargetType GetTargetType() const { return Type; }

	/** A projectile struck this target. Scores per type (only while the stage is running). */
	void HandleHit(const FHitResult& Hit);

	/** Clear hits / stand steel back up for a fresh run. */
	void ResetTarget();

	// --- queries used by the stage scorer ---
	int32 NumPaperHits() const { return HitValues.Num(); }
	int32 BestTwoSum() const;
	void CountBestTwoZones(int32& OutA, int32& OutC, int32& OutD) const;
	int32 GetNoShootHits() const { return NoShootHits; }
	bool IsSteelDown() const { return bDown; }

protected:
	virtual void BeginPlay() override;

	void ApplyVisuals();
	int32 ZoneValueForHit(const FHitResult& Hit) const;
	void KnockDown();

	UPROPERTY()
	UStaticMeshComponent* Mesh;

	/** Dark central patch marking the A-zone on paper targets. */
	UPROPERTY()
	UStaticMeshComponent* AZoneMesh;

	ETargetType Type = ETargetType::Paper;

	/** Per-hit zone values (5/3/1) for paper targets. */
	TArray<int32> HitValues;
	int32 NoShootHits = 0;
	bool bDown = false;

	UPROPERTY()
	class USoundBase* SteelSound = nullptr;

	UPROPERTY()
	class USoundBase* PaperSound = nullptr;

	FRotator InitialRotation = FRotator::ZeroRotator;

	// Half-extents of the paper face (UU), set in ApplyVisuals.
	float HalfWidth = 27.5f;
	float HalfHeight = 42.5f;
};
