#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RunnerGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button events
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayButtonClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitButtonClicked);
	}

	// Set title
	if (TitleText)
	{
		TitleText->SetText(FText::FromString("GU-GO RUNNER"));
	}

	// TODO: Load and display high score
	if (HighScoreText)
	{
		HighScoreText->SetText(FText::FromString("High Score: 0"));
	}
}

void UMainMenuWidget::OnPlayButtonClicked()
{
	// Get game mode and start game
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		// Remove this widget
		RemoveFromParent();
		
		// Start the game
		GameMode->StartGame();
	}
}

void UMainMenuWidget::OnQuitButtonClicked()
{
	// Quit the game
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);
	}
}