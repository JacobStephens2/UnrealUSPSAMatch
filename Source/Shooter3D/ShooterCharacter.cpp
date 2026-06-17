#include "ShooterCharacter.h"
#include "ShooterProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// First-person camera at eye height; it owns the view rotation.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	// A simple cube "gun" so the player sees they are holding something.
	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(Camera);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GunMesh->SetRelativeLocation(FVector(40.f, 18.f, -18.f));
	GunMesh->SetRelativeScale3D(FVector(0.5f, 0.12f, 0.12f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		GunMesh->SetStaticMesh(CubeMesh.Object);
	}

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->AirControl = 0.3f;
	GetCharacterMovement()->JumpZVelocity = 450.f;

	ProjectileClass = AShooterProjectile::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> ShotSound(TEXT("/Game/Audio/shot.shot"));
	if (ShotSound.Succeeded())
	{
		FireSound = ShotSound.Object;
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FireTimer > 0.f)
	{
		FireTimer -= DeltaSeconds;
	}
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::TurnRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUpRate);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::Fire);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Tap-to-shoot: touches in the virtual-joystick zones are consumed by the
	// sticks, so any other tap (the upper play area) lands here and fires.
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AShooterCharacter::OnTouchPressed);
}

void AShooterCharacter::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	Fire();
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AShooterCharacter::TurnRate(float Value)
{
	AddControllerYawInput(Value);
}

void AShooterCharacter::LookUpRate(float Value)
{
	AddControllerPitchInput(Value);
}

void AShooterCharacter::Fire()
{
	if (FireTimer > 0.f || !ProjectileClass)
	{
		return;
	}
	FireTimer = FireCooldown;

	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const FRotator AimRot = Camera->GetComponentRotation();
	const FVector MuzzleLoc = Camera->GetComponentLocation() + AimRot.Vector() * 80.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;
	Params.Instigator = this;

	if (AShooterProjectile* Proj = GetWorld()->SpawnActor<AShooterProjectile>(ProjectileClass, MuzzleLoc, AimRot, Params))
	{
		Proj->FireInDirection(AimRot.Vector());
	}
}
