#include "ShooterHUD.h"
#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWeakWidget.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const FSlateFontInfo ScoreFont = FCoreStyle::GetDefaultFontStyle("Bold", 32);
	const FSlateFontInfo FireFont = FCoreStyle::GetDefaultFontStyle("Bold", 28);

	OverlayWidget =
		SNew(SOverlay)

		// Score, top-centre.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Top)
		.Padding(0, 30, 0, 0)
		[
			SNew(STextBlock)
			.Font(ScoreFont)
			.ColorAndOpacity(FLinearColor::White)
			.ShadowOffset(FVector2D(2, 2))
			.Text_UObject(this, &AShooterHUD::GetScoreText)
		]

		// Control hint, bottom-centre.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Bottom)
		.Padding(0, 0, 0, 24)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f))
			.ShadowOffset(FVector2D(1, 1))
			.Text(FText::FromString(TEXT("Tap anywhere to shoot  •  drag corners to move / look")))
		]

		// FIRE button, bottom-right (thumb reach on a phone).
		+ SOverlay::Slot()
		.HAlign(HAlign_Right).VAlign(VAlign_Bottom)
		.Padding(0, 0, 60, 90)
		[
			SNew(SBox).WidthOverride(180).HeightOverride(180)
			[
				SNew(SButton)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.OnClicked(FOnClicked::CreateUObject(this, &AShooterHUD::OnFireClicked))
				[
					SNew(STextBlock)
					.Font(FireFont)
					.Text(FText::FromString(TEXT("FIRE")))
				]
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(
		SNew(SWeakWidget).PossiblyNullContent(OverlayWidget.ToSharedRef()), 10);

	// Let the FIRE button receive touches while move/look input still reaches the pawn.
	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

FReply AShooterHUD::OnFireClicked()
{
	if (AShooterCharacter* Shooter = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Shooter->Fire();
	}
	return FReply::Handled();
}

FText AShooterHUD::GetScoreText() const
{
	int32 Score = 0;
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Score = GM->GetScore();
	}
	return FText::FromString(FString::Printf(TEXT("SCORE  %d"), Score));
}
