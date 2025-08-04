#include "RunnerHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "RunnerGameMode.h"
#include "Kismet/GameplayStatics.h"

void URunnerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Hide game over panel initially
	if (GameOverPanel)
	{
		GameOverPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Hide pause menu initially
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Bind button clicks
	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &URunnerHUD::OnRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &URunnerHUD::OnMainMenuClicked);
	}

	if (PauseButton)
	{
		PauseButton->OnClicked.AddDynamic(this, &URunnerHUD::OnPauseClicked);
	}
}

void URunnerHUD::UpdateScore(int32 NewScore)
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), NewScore)));
	}
}

void URunnerHUD::UpdateDistance(float NewDistance)
{
	if (DistanceText)
	{
		DistanceText->SetText(FText::FromString(FString::Printf(TEXT("Distance: %.0fm"), NewDistance)));
	}
}

void URunnerHUD::ShowGameOver()
{
	if (GameOverPanel)
	{
		GameOverPanel->SetVisibility(ESlateVisibility::Visible);
		
		// Update final score
		if (FinalScoreText)
		{
			if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
			{
				FinalScoreText->SetText(FText::FromString(
					FString::Printf(TEXT("Final Score: %d\nDistance: %.0fm"), 
					GameMode->GetScore(), 
					GameMode->GetDistanceRun())
				));
			}
		}
	}
}

void URunnerHUD::HideGameOver()
{
	if (GameOverPanel)
	{
		GameOverPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URunnerHUD::OnRestartClicked()
{
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->RestartGame();
	}
}

void URunnerHUD::OnMainMenuClicked()
{
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->ReturnToMainMenu();
	}
}

void URunnerHUD::UpdateCoins(int32 NewCoins)
{
	if (CoinsText)
	{
		CoinsText->SetText(FText::Format(NSLOCTEXT("HUD", "Coins", "Coins: {0}"), FText::AsNumber(NewCoins)));
	}
}

void URunnerHUD::UpdateSteps(int32 NewSteps)
{
	if (StepsText)
	{
		StepsText->SetText(FText::Format(NSLOCTEXT("HUD", "Steps", "Steps: {0}"), FText::AsNumber(NewSteps)));
	}
}

void URunnerHUD::ShowPauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void URunnerHUD::HidePauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URunnerHUD::OnPauseClicked()
{
	ShowPauseMenu();
}