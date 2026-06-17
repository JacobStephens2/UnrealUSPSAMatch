#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Input/Reply.h"
#include "ShooterHUD.generated.h"

class SWidget;

UCLASS()
class SHOOTER3D_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	FReply OnFireClicked();
	FText GetScoreText() const;

	TSharedPtr<SWidget> OverlayWidget;
};
