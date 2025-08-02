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
	
	CurrentGameSpeed = InitialGameSpeed;
}

void ARunnerGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay - Starting Game"));
	
	// FOR TESTING: Skip main menu and start game directly
	CurrentGameState = EGameState::MainMenu;
	
	// Auto-start the game for testing
	FTimerHandle StartGameTimer;
	GetWorld()->GetTimerManager().SetTimer(StartGameTimer, [this]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Auto-starting game for testing"));
		StartGame();
	}, 1.0f, false);
	
	// Comment out main menu for now
	/*if (MainMenuWidgetClass)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			UUserWidget* MainMenu = CreateWidget<UUserWidget>(PlayerController, MainMenuWidgetClass);
			if (MainMenu)
			{
				MainMenu->AddToViewport();
				PlayerController->SetShowMouseCursor(true);
				PlayerController->SetInputMode(FInputModeUIOnly());
			}
		}
	}*/
}

void ARunnerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
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
		PlayerCharacter->ForwardSpeed = 0.0f;
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		PlayerCharacter->DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}
	
	// Show game over UI
	if (RunnerHUD)
	{
		RunnerHUD->ShowGameOver();
	}
	
	// Enable mouse for menu interaction
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetInputMode(FInputModeUIOnly());
	}
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
	
	// Start player movement
	if (PlayerCharacter)
	{
		PlayerCharacter->ForwardSpeed = CurrentGameSpeed;
		PlayerCharacter->GetCharacterMovement()->MaxWalkSpeed = CurrentGameSpeed;
		UE_LOG(LogTemp, Warning, TEXT("Player speed set to: %f"), CurrentGameSpeed);
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