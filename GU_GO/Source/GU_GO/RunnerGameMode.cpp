#include "RunnerGameMode.h"
#include "RunnerCharacter.h"
#include "RunnerHUD.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

ARunnerGameMode::ARunnerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Set default pawn class to our Runner Character
	DefaultPawnClass = ARunnerCharacter::StaticClass();
	
	// Set default HUD class to use widget-based HUD
	
	CurrentGameSpeed = InitialGameSpeed;
}

void ARunnerGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay - Auto-starting infinite runner"));
	
	// IMMEDIATE: Set input mode to capture mouse right away
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogTemp, Warning, TEXT("Set input mode immediately in BeginPlay"));
	}
	
	// Start countdown sequence instead of immediately starting
	StartCountdown();
}

void ARunnerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Handle restart input during game over
	if (CurrentGameState == EGameState::GameOver)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			// Check for R key press to restart
			if (PlayerController->IsInputKeyDown(EKeys::R))
			{
				UE_LOG(LogTemp, Warning, TEXT("RESTART: R key pressed, restarting game"));
				RestartGame();
				return;
			}
		}
	}
	
	if (CurrentGameState == EGameState::Playing && !bIsGameOver)
	{
		// Update time and distance
		TimePlayed += DeltaTime;
		DistanceRun += (CurrentGameSpeed * DeltaTime) / 100.0f; // Convert to meters
		
		// Increase game speed over time
		CurrentGameSpeed = FMath::Min(CurrentGameSpeed + (SpeedIncreaseRate * DeltaTime), MaxGameSpeed);
		
		// Update player speed
		if (PlayerCharacter)
		{
			PlayerCharacter->ForwardSpeed = CurrentGameSpeed;
			PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = CurrentGameSpeed;
		}
		
		// Add score based on distance
		if (FMath::IsNearlyEqual(FMath::Fmod(DistanceRun, 10.0f), 0.0f, 0.5f))
		{
			AddScore(10);
		}
		
		// Update HUD
		if (RunnerHUD)
		{
			RunnerHUD->UpdateDistance(DistanceRun);
			
			// Update coins and steps from player character
			if (PlayerCharacter)
			{
				RunnerHUD->UpdateCoins(PlayerCharacter->CoinsCollected);
				RunnerHUD->UpdateSteps(PlayerCharacter->StepCount);
			}
		}
	}
}

void ARunnerGameMode::AddScore(int32 Points)
{
	Score += Points;
	
	// Update HUD
	if (RunnerHUD)
	{
		RunnerHUD->UpdateScore(Score);
	}
}

void ARunnerGameMode::GameOver()
{
	bIsGameOver = true;
	CurrentGameState = EGameState::GameOver;
	
	// Stop player movement
	if (PlayerCharacter)
	{
		PlayerCharacter->CurrentGameSpeed = 0.0f; // Stop treadmill
		PlayerCharacter->ForwardSpeed = 0.0f;
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		PlayerCharacter->DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}
	
	// Show game over UI
	if (RunnerHUD)
	{
		RunnerHUD->ShowGameOver();
	}
	
	// Display game over text on screen
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, 
			TEXT("GAME OVER!"), true, FVector2D(4.0f, 4.0f));
		GEngine->AddOnScreenDebugMessage(-2, 10.0f, FColor::White, 
			TEXT("Press R to Restart"), true, FVector2D(2.5f, 2.5f));
		GEngine->AddOnScreenDebugMessage(-3, 10.0f, FColor::Yellow, 
			FString::Printf(TEXT("Final Score: %d | Steps: %d"), Score, PlayerCharacter ? PlayerCharacter->StepCount : 0), 
			true, FVector2D(1.8f, 1.8f));
	}
	
	// Keep game input active for restart key
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly()); // Keep game input for restart
	}
	
	UE_LOG(LogTemp, Error, TEXT("GAME OVER: Final Score=%d, Steps=%d"), Score, PlayerCharacter ? PlayerCharacter->StepCount : 0);
}

void ARunnerGameMode::RestartGame()
{
	// Reset game state
	Score = 0;
	DistanceRun = 0.0f;
	TimePlayed = 0.0f;
	CurrentGameSpeed = InitialGameSpeed;
	bIsGameOver = false;
	
	// Restart level
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
}

void ARunnerGameMode::StartGame()
{
	UE_LOG(LogTemp, Warning, TEXT("StartGame called - Changing to Playing state"));
	
	// Change state to playing
	CurrentGameState = EGameState::Playing;
	
	// Get reference to player character
	PlayerCharacter = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	UE_LOG(LogTemp, Warning, TEXT("Player Character: %s"), PlayerCharacter ? TEXT("Found") : TEXT("NULL"));
	
	// Create and show game HUD
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	UE_LOG(LogTemp, Warning, TEXT("Player Controller: %s"), PlayerController ? TEXT("Found") : TEXT("NULL"));
	
	if (PlayerController)
	{
		// Remove any existing widgets
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
		UE_LOG(LogTemp, Warning, TEXT("Set input mode to Game Only"));
		
		// Create game HUD if we have the class
		if (HUDWidgetClass)
		{
			RunnerHUD = CreateWidget<URunnerHUD>(PlayerController, HUDWidgetClass);
			if (RunnerHUD)
			{
				RunnerHUD->AddToViewport();
				RunnerHUD->UpdateScore(Score);
				RunnerHUD->UpdateDistance(DistanceRun);
				RunnerHUD->HideGameOver();
				UE_LOG(LogTemp, Warning, TEXT("HUD Created and added to viewport"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No HUD Widget Class set!"));
		}
	}
	
	// Start player movement and ensure treadmill speed is set
	if (PlayerCharacter)
	{
		PlayerCharacter->ForwardSpeed = CurrentGameSpeed;
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = CurrentGameSpeed;
		PlayerCharacter->CurrentGameSpeed = CurrentGameSpeed; // Ensure treadmill speed is set
		UE_LOG(LogTemp, Error, TEXT("GAME_STARTED: Player speed set to: %.1f"), CurrentGameSpeed);
	}
}

void ARunnerGameMode::PauseGame()
{
	if (CurrentGameState == EGameState::Playing)
	{
		CurrentGameState = EGameState::Paused;
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		
		// Show pause menu UI
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			PlayerController->SetShowMouseCursor(true);
			PlayerController->SetInputMode(FInputModeGameAndUI());
		}
	}
}

void ARunnerGameMode::ResumeGame()
{
	if (CurrentGameState == EGameState::Paused)
	{
		CurrentGameState = EGameState::Playing;
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		
		// Hide pause menu UI
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			PlayerController->SetShowMouseCursor(false);
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
	}
}

void ARunnerGameMode::ReturnToMainMenu()
{
	// Reset game state
	Score = 0;
	DistanceRun = 0.0f;
	TimePlayed = 0.0f;
	CurrentGameSpeed = InitialGameSpeed;
	bIsGameOver = false;
	CurrentGameState = EGameState::MainMenu;
	
	// Reload level to reset everything
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
}

void ARunnerGameMode::StartCountdown()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting countdown sequence"));
	
	// Set to countdown state
	CurrentGameState = EGameState::Countdown;
	CountdownNumber = 3;
	
	// Player will be in idle state - no movement during countdown
	// CurrentGameSpeed will be set to 0 by player's Tick()
	
	// Start countdown timer - trigger every 1 second
	GetWorld()->GetTimerManager().SetTimer(CountdownTimer, [this]()
	{
		FString CountdownText = GetCountdownText();
		UE_LOG(LogTemp, Warning, TEXT("Countdown: %s"), *CountdownText);
		
		// Display countdown on screen for 0.8 seconds (shorter than 1s interval)
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.8f, FColor::White, CountdownText, true, FVector2D(3.0f, 3.0f));
		}
		
		CountdownNumber--;
		
		if (CountdownNumber < 0)
		{
			// Countdown finished - start game!
			GetWorld()->GetTimerManager().ClearTimer(CountdownTimer);
			StartGame();
		}
	}, 1.0f, true);
}

FString ARunnerGameMode::GetCountdownText() const
{
	switch (CountdownNumber)
	{
		case 3: return TEXT("3");
		case 2: return TEXT("2"); 
		case 1: return TEXT("1");
		case 0: return TEXT("RUN!");
		default: return TEXT("");
	}
}