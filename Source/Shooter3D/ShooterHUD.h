#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Input/Reply.h"
#include "ShooterHUD.generated.h"

class SWidget;
class AShooterGameMode;

UCLASS()
class SHOOTER3D_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	FReply OnStartClicked();

	TSharedPtr<SWidget> OverlayWidget;
	TWeakObjectPtr<AShooterGameMode> GameModeRef;
};
