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
		// Check if player died and immediately stop gameplay
		if (PlayerCharacter && PlayerCharacter->bIsDead)
		{
			UE_LOG(LogTemp, Warning, TEXT("GAMEMODE: Player death detected, stopping treadmill system"));
			CurrentGameSpeed = 0.0f;
			// Don't call GameOver() immediately - let the death sequence handle it
			return;
		}
		
		// Update time and distance
		TimePlayed += DeltaTime;
		DistanceRun += (CurrentGameSpeed * DeltaTime) / 100.0f; // Convert to meters
		
		// Increase game speed over time
		CurrentGameSpeed = FMath::Min(CurrentGameSpeed + (SpeedIncreaseRate * DeltaTime), MaxGameSpeed);
		
		// Update player speed (only if player is alive)
		if (PlayerCharacter && !PlayerCharacter->bIsDead)
		{
			PlayerCharacter->ForwardSpeed = CurrentGameSpeed;
			PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = CurrentGameSpeed;
		}
		
		// Add score based on distance (more reliable calculation)
		static float LastScoredDistance = 0.0f;
		if (DistanceRun >= LastScoredDistance + 10.0f)
		{
			int32 PointsToAdd = ((int32)(DistanceRun / 10.0f) - (int32)(LastScoredDistance / 10.0f)) * 10;
			AddScore(PointsToAdd);
			LastScoredDistance = FMath::FloorToFloat(DistanceRun / 10.0f) * 10.0f;
		}
		
		// Update HUD - only Coins, Score, and Steps
		if (RunnerHUD)
		{
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
	
	// Final score update based on distance
	int32 FinalDistanceScore = ((int32)(DistanceRun / 10.0f)) * 10;
	if (FinalDistanceScore > Score)
	{
		Score = FinalDistanceScore; // Ensure score matches distance
	}
	
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
	
	// Debug messages removed - using UI instead
	
	// Enable UI interaction for game over screen
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetInputMode(FInputModeGameAndUI()); // Allow both game input (R key) and UI clicks
	}
	
	UE_LOG(LogTemp, Error, TEXT("GAME OVER: Score=%d, Coins=%d, Steps=%d"), 
		Score, 
		PlayerCharacter ? PlayerCharacter->CoinsCollected : 0,
		PlayerCharacter ? PlayerCharacter->StepCount : 0);
}

void ARunnerGameMode::RestartGame()
{
	UE_LOG(LogTemp, Warning, TEXT("RESTART: RestartGame called"));
	
	// Reset game state
	Score = 0;
	DistanceRun = 0.0f;
	TimePlayed = 0.0f;
	CurrentGameSpeed = InitialGameSpeed;
	bIsGameOver = false;
	CurrentGameState = EGameState::Countdown;
	
	// Reset player character
	if (PlayerCharacter)
	{
		PlayerCharacter->ResetDeathState();
	}
	
	// Unpause the game first
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	
	// Hide all UI panels
	if (RunnerHUD)
	{
		RunnerHUD->HidePauseMenu();
		RunnerHUD->HideContinuePrompt();
		RunnerHUD->HideGameOver();
	}
	
	// Restart level safely
	FString CurrentLevelName = GetWorld()->GetName();
	UE_LOG(LogTemp, Warning, TEXT("RESTART: Reloading level: %s"), *CurrentLevelName);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*CurrentLevelName));
}

void ARunnerGameMode::StartGame()
{
	UE_LOG(LogTemp, Error, TEXT("=== START GAME CALLED ==="));
	
	// Safety checks
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("STARTGAME_ERROR: No valid world!"));
		return;
	}
	
	// Change state to playing
	CurrentGameState = EGameState::Playing;
	UE_LOG(LogTemp, Warning, TEXT("STARTGAME: State changed to Playing"));
	
	// Get reference to player character
	PlayerCharacter = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	UE_LOG(LogTemp, Warning, TEXT("STARTGAME: Player Character: %s"), PlayerCharacter ? TEXT("Found") : TEXT("NULL"));
	
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("STARTGAME_ERROR: No player character found! Game may not work properly."));
	}
	
	// Create and show game HUD
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	UE_LOG(LogTemp, Warning, TEXT("STARTGAME: Player Controller: %s"), PlayerController ? TEXT("Found") : TEXT("NULL"));
	
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
	// Check if player is dead or continue prompt is active - don't allow manual pause
	if (PlayerCharacter && (PlayerCharacter->bIsDead || PlayerCharacter->bContinueOffered))
	{
		UE_LOG(LogTemp, Warning, TEXT("PAUSE_BLOCKED: Cannot pause during death/continue sequence"));
		return;
	}
	
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
		
		// Show the pause menu through HUD
		if (RunnerHUD)
		{
			// Make sure continue prompt is hidden first
			RunnerHUD->HideContinuePrompt();
			RunnerHUD->ShowPauseMenu();
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
		
		// Hide the pause menu through HUD
		if (RunnerHUD)
		{
			RunnerHUD->HidePauseMenu();
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
	
	// Load main menu level
	UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("MainMenuLevel")));
}

void ARunnerGameMode::StartCountdown()
{
	UE_LOG(LogTemp, Error, TEXT("=== STARTING COUNTDOWN SEQUENCE ==="));
	
	// Safety check - make sure we have a valid world
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("COUNTDOWN_ERROR: No valid world!"));
		return;
	}
	
	// Set to countdown state
	CurrentGameState = EGameState::Countdown;
	CountdownNumber = 3;
	
	UE_LOG(LogTemp, Warning, TEXT("COUNTDOWN: State set, starting timer"));
	
	// Player will be in idle state - no movement during countdown
	// CurrentGameSpeed will be set to 0 by player's Tick()
	
	// Start countdown timer - trigger every 1 second
	GetWorld()->GetTimerManager().SetTimer(CountdownTimer, [this]()
	{
		UE_LOG(LogTemp, Error, TEXT("COUNTDOWN_TIMER: Executing countdown step %d"), CountdownNumber);
		
		if (!IsValid(this))
		{
			UE_LOG(LogTemp, Error, TEXT("COUNTDOWN_ERROR: GameMode is no longer valid!"));
			return;
		}
		
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
			UE_LOG(LogTemp, Error, TEXT("COUNTDOWN_FINISHED: Clearing timer and starting game"));
			// Countdown finished - start game!
			if (GetWorld())
			{
				GetWorld()->GetTimerManager().ClearTimer(CountdownTimer);
			}
			StartGame();
		}
	}, 1.0f, true);
	
	UE_LOG(LogTemp, Warning, TEXT("COUNTDOWN: Timer started successfully"));
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