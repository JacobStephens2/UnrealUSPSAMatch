#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class SHOOTER3D_API AShooterProjectile : public AActor
{
	GENERATED_BODY()

public:
	AShooterProjectile();

	/** Launch the projectile along the given direction. */
	void FireInDirection(const FVector& ShootDirection);

	/** Mark this as an enemy shot that damages the player instead of scoring a target. */
	void ConfigureAsEnemyShot(int32 InDamage);

protected:
	bool bEnemyShot = false;
	int32 Damage = 0;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* Movement;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
