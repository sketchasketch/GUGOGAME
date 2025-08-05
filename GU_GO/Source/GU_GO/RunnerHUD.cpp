#include "RunnerHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "RunnerGameMode.h"
#include "RunnerCharacter.h"
#include "Kismet/GameplayStatics.h"

void URunnerHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Debug: UE_LOG(LogTemp, Error, TEXT("=== RUNNERHUD NATIVE CONSTRUCT CALLED ==="));

	// Hide game over panel initially
	if (GameOverPanel)
	{
		GameOverPanel->SetVisibility(ESlateVisibility::Collapsed);
	}


	// Hide continue prompt initially
	if (ContinuePromptPanel)
	{
		ContinuePromptPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Hide pause menu initially
	if (PauseMenuPanel)
	{
		PauseMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Bind button clicks with debug logging
	UE_LOG(LogTemp, Error, TEXT("BINDING BUTTONS:"));
	UE_LOG(LogTemp, Error, TEXT("  RestartButton: %s"), RestartButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  MainMenuButton: %s"), MainMenuButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  GamePauseBtn: %s"), GamePauseBtn ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  ResumeButton: %s"), ResumeButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  ContinueButton: %s"), ContinueButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  DeclineButton: %s"), DeclineButton ? TEXT("EXISTS") : TEXT("NULL"));
	
	UE_LOG(LogTemp, Error, TEXT("CHECKING TEXT WIDGETS:"));
	UE_LOG(LogTemp, Error, TEXT("  ScoreText: %s"), ScoreText ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  DistanceText: %s"), DistanceText ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  PlayerCoins: %s"), PlayerCoins ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  PlayerSteps: %s"), PlayerSteps ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("  PlayerGems: %s"), PlayerGems ? TEXT("EXISTS") : TEXT("NULL"));

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &URunnerHUD::OnRestartClicked);
		UE_LOG(LogTemp, Warning, TEXT("RestartButton bound successfully"));
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &URunnerHUD::OnMainMenuClicked);
		UE_LOG(LogTemp, Warning, TEXT("MainMenuButton bound successfully"));
	}

	if (GamePauseBtn)
	{
		GamePauseBtn->OnClicked.AddDynamic(this, &URunnerHUD::OnPauseClicked);
		UE_LOG(LogTemp, Warning, TEXT("GamePauseBtn bound successfully"));
	}

	// Bind continue prompt buttons
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &URunnerHUD::OnContinueClicked);
	}

	if (DeclineButton)
	{
		DeclineButton->OnClicked.AddDynamic(this, &URunnerHUD::OnDeclineClicked);
	}

	// Bind pause menu buttons
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &URunnerHUD::OnResumeClicked);
	}

	if (RestartFromPauseButton)
	{
		RestartFromPauseButton->OnClicked.AddDynamic(this, &URunnerHUD::OnRestartFromPauseClicked);
	}

	if (MainMenuFromPauseButton)
	{
		MainMenuFromPauseButton->OnClicked.AddDynamic(this, &URunnerHUD::OnMainMenuFromPauseClicked);
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
				// Get player character for coins and steps
				if (ARunnerCharacter* PlayerCharacter = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
				{
					FinalScoreText->SetText(FText::FromString(
						FString::Printf(TEXT("Final Score: %d\nCoins: %d\nSteps: %d"), 
						GameMode->GetScore(), 
						PlayerCharacter->CoinsCollected,
						PlayerCharacter->StepCount)
					));
				}
				else
				{
					FinalScoreText->SetText(FText::FromString(
						FString::Printf(TEXT("Final Score: %d"), GameMode->GetScore())
					));
				}
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
	if (PlayerCoins)
	{
		PlayerCoins->SetText(FText::Format(NSLOCTEXT("HUD", "Coins", "Coins: {0}"), FText::AsNumber(NewCoins)));
	}
}

void URunnerHUD::UpdateSteps(int32 NewSteps)
{
	if (PlayerSteps)
	{
		PlayerSteps->SetText(FText::Format(NSLOCTEXT("HUD", "Steps", "Steps: {0}"), FText::AsNumber(NewSteps)));
	}
}


void URunnerHUD::OnPauseClicked()
{
	UE_LOG(LogTemp, Error, TEXT("***** PAUSE BUTTON CLICKED - C++ FUNCTION CALLED *****"));
	
	// Get game mode and trigger pause
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		UE_LOG(LogTemp, Error, TEXT("Found GameMode, calling PauseGame()"));
		GameMode->PauseGame();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode is NULL or wrong type!"));
	}
}

void URunnerHUD::ShowContinuePrompt(int32 GemsOwned, int32 ContinueCost)
{
	if (ContinuePromptPanel)
	{
		ContinuePromptPanel->SetVisibility(ESlateVisibility::Visible);
		
		// Update prompt text
		if (ContinuePromptText)
		{
			ContinuePromptText->SetText(FText::FromString(
				TEXT("You died! Do you want to continue?\n\nYou'll lose 50% of your score and distance.")
			));
		}
		
		// Update gems info
		if (GemsInfoText)
		{
			GemsInfoText->SetText(FText::FromString(
				FString::Printf(TEXT("Cost: %d Gems\nYou have: %d Gems"), ContinueCost, GemsOwned)
			));
		}
		
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_UI: Showing continue prompt"));
	}
}

void URunnerHUD::HideContinuePrompt()
{
	if (ContinuePromptPanel)
	{
		ContinuePromptPanel->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_UI: Hiding continue prompt"));
	}
}

void URunnerHUD::OnContinueClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("CONTINUE_UI: Continue button clicked"));
	
	// Get player character and accept continue
	if (ARunnerCharacter* PlayerCharacter = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerCharacter->AcceptContinue();
	}
	
	// Hide the prompt
	HideContinuePrompt();
}

void URunnerHUD::OnDeclineClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("CONTINUE_UI: Decline button clicked"));
	
	// Get player character and decline continue
	if (ARunnerCharacter* PlayerCharacter = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerCharacter->DeclineContinue();
	}
	
	// Hide the prompt
	HideContinuePrompt();
}

void URunnerHUD::UpdateGems(int32 NewGems)
{
	if (PlayerGems)
	{
		PlayerGems->SetText(FText::FromString(FString::Printf(TEXT("💎 %d"), NewGems)));
	}
}

void URunnerHUD::ShowPauseMenu()
{
	if (PauseMenuPanel)
	{
		PauseMenuPanel->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("PAUSE_UI: Showing pause menu"));
	}
}

void URunnerHUD::HidePauseMenu()
{
	if (PauseMenuPanel)
	{
		PauseMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Warning, TEXT("PAUSE_UI: Hiding pause menu"));
	}
}

void URunnerHUD::OnResumeClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("PAUSE_UI: Resume button clicked"));
	
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->ResumeGame();
	}
	
	HidePauseMenu();
}

void URunnerHUD::OnRestartFromPauseClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("PAUSE_UI: Restart button clicked"));
	
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->RestartGame();
	}
}

void URunnerHUD::OnMainMenuFromPauseClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("PAUSE_UI: Main menu button clicked"));
	
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->ReturnToMainMenu();
	}
}