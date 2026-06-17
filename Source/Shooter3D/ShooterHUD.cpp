#include "ShooterHUD.h"
#include "ShooterGameMode.h"
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

	GameModeRef = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));

	const FSlateFontInfo StatFont   = FCoreStyle::GetDefaultFontStyle("Bold", 34);
	const FSlateFontInfo StatusFont = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo GoFont     = FCoreStyle::GetDefaultFontStyle("Bold", 96);
	const FSlateFontInfo ResultFont = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo BtnFont    = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo HintFont   = FCoreStyle::GetDefaultFontStyle("Regular", 19);

	auto GM = [this]() -> AShooterGameMode* { return GameModeRef.Get(); };

	// TIME + POINTS, shown once the run is underway.
	auto StatsVisibility = [this]() {
		AShooterGameMode* G = GameModeRef.Get();
		return (G && G->GetState() != EStageState::PreStart) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
	};

	OverlayWidget =
		SNew(SOverlay)

		// --- Top: TIME / POINTS ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Top)
		.Padding(0, 24, 0, 0)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda(StatsVisibility)
			+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0)
			[
				SNew(STextBlock).Font(StatFont).ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(2, 2))
				.Text_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					return FText::FromString(FString::Printf(TEXT("TIME  %.2f"), G ? G->GetTime() : 0.f));
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0)
			[
				SNew(STextBlock).Font(StatFont).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.3f)).ShadowOffset(FVector2D(2, 2))
				.Text_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					return FText::FromString(FString::Printf(TEXT("PTS  %d"), G ? G->GetPoints() : 0));
				})
			]
		]

		// --- Centre: status text + results + START/RUN-AGAIN button ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(8)
			[
				SNew(STextBlock).Font(StatusFont).Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(2, 2))
				.Text_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					return FText::FromString(G ? G->GetStatusText() : FString());
				})
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(8)
			[
				SNew(STextBlock).Font(ResultFont).Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor(0.8f, 1.f, 0.8f)).ShadowOffset(FVector2D(2, 2))
				.Visibility_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					return (G && G->GetState() == EStageState::Complete) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
				.Text_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					return FText::FromString(G ? G->GetResultText() : FString());
				})
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(16)
			[
				SNew(SButton)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.ContentPadding(FMargin(44, 18))
				.OnClicked(FOnClicked::CreateUObject(this, &AShooterHUD::OnStartClicked))
				.Visibility_Lambda([this]() {
					AShooterGameMode* G = GameModeRef.Get();
					const bool bShow = G && (G->GetState() == EStageState::PreStart || G->GetState() == EStageState::Complete);
					return bShow ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(STextBlock).Font(BtnFont)
					.Text_Lambda([this]() {
						AShooterGameMode* G = GameModeRef.Get();
						return FText::FromString((G && G->GetState() == EStageState::Complete) ? TEXT("RUN AGAIN") : TEXT("START"));
					})
				]
			]
		]

		// --- Big "GO!" buzzer flash ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(GoFont).ColorAndOpacity(FLinearColor(0.3f, 1.f, 0.3f)).ShadowOffset(FVector2D(3, 3))
			.Text(FText::FromString(TEXT("GO!")))
			.Visibility_Lambda([this]() {
				AShooterGameMode* G = GameModeRef.Get();
				return (G && G->ShowGo()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
		]

		// --- Bottom hint ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Bottom)
		.Padding(0, 0, 0, 22)
		[
			SNew(STextBlock).Font(HintFont).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f)).ShadowOffset(FVector2D(1, 1))
			.Text(FText::FromString(TEXT("Tap to shoot  •  drag corners to move / look")))
		];

	GEngine->GameViewport->AddViewportWidgetContent(
		SNew(SWeakWidget).PossiblyNullContent(OverlayWidget.ToSharedRef()), 10);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

FReply AShooterHUD::OnStartClicked()
{
	if (AShooterGameMode* G = GameModeRef.Get())
	{
		G->StartStage();
	}
	return FReply::Handled();
}
