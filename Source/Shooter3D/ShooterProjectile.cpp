#include "ShooterProjectile.h"
#include "ShooterTarget.h"
#include "ShooterCharacter.h"
#include "ShooterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AShooterProjectile::AShooterProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.0f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentHit.AddDynamic(this, &AShooterProjectile::OnHit);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.25f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -12.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 3000.0f;
	Movement->MaxSpeed = 3000.0f;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.15f;

	InitialLifeSpan = 3.0f;
}

void AShooterProjectile::FireInDirection(const FVector& ShootDirection)
{
	Movement->Velocity = ShootDirection.GetSafeNormal() * Movement->InitialSpeed;
}

void AShooterProjectile::ConfigureAsEnemyShot(int32 InDamage)
{
	bEnemyShot = true;
	Damage = InDamage;
	// Enemy rounds travel a touch slower so the player can see and dodge them.
	Movement->InitialSpeed = 1500.f;
	Movement->MaxSpeed = 1500.f;
	Mesh->SetRelativeScale3D(FVector(0.4f)); // a bit bigger / readable
}

void AShooterProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (bEnemyShot)
	{
		if (Cast<AShooterCharacter>(OtherActor))
		{
			if (AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				GM->PlayerHit(Damage);
			}
		}
		Destroy();
		return;
	}

	if (AShooterTarget* Target = Cast<AShooterTarget>(OtherActor))
	{
		Target->HandleHit(Hit);
	}
	Destroy();
}
