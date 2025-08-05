#include "MainMenuGameMode.h"
#include "MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// Set input mode for UI interaction
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetInputMode(FInputModeUIOnly());
		UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Set UI-only input mode"));
	}
	
	// Create and show main menu widget
	UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: MainMenuWidgetClass = %s"), MainMenuWidgetClass ? *MainMenuWidgetClass->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: PlayerController = %s"), PlayerController ? TEXT("Valid") : TEXT("NULL"));
	
	if (MainMenuWidgetClass && PlayerController)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(PlayerController, MainMenuWidgetClass);
		UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: CreateWidget result = %s"), MainMenuWidget ? TEXT("SUCCESS") : TEXT("FAILED"));
		
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Main menu widget created and added to viewport"));
			
			// Try to cast to our specific widget type for debugging
			if (UMainMenuWidget* TypedWidget = Cast<UMainMenuWidget>(MainMenuWidget))
			{
				UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Successfully cast to UMainMenuWidget"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: Failed to cast to UMainMenuWidget - using generic UUserWidget"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: MainMenuWidgetClass not set or PlayerController not found!"));
	}
}

void AMainMenuGameMode::StartGame()
{
	UE_LOG(LogTemp, Error, TEXT("***** MAIN MENU START GAME CALLED *****"));
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Starting game - loading %s"), *GameLevelName.ToString());
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Current world name: %s"), GetWorld() ? *GetWorld()->GetName() : TEXT("NULL"));
	
	// Hide main menu
	if (MainMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Removing main menu widget"));
		MainMenuWidget->RemoveFromParent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: MainMenuWidget is NULL!"));
	}
	
	// Load game level
	UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: About to call UGameplayStatics::OpenLevel with: %s"), *GameLevelName.ToString());
	
	// Try multiple level name formats to ensure it works
	FName LevelToLoad = GameLevelName;
	if (GameLevelName.ToString().Contains(TEXT("/")))
	{
		// Already has path
		UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Using full path: %s"), *LevelToLoad.ToString());
	}
	else
	{
		// Add path prefix
		LevelToLoad = FName(*FString::Printf(TEXT("/Game/Maps/%s"), *GameLevelName.ToString()));
		UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Added path prefix: %s"), *LevelToLoad.ToString());
	}
	
	UGameplayStatics::OpenLevel(GetWorld(), LevelToLoad);
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: OpenLevel call completed"));
}

void AMainMenuGameMode::QuitGame()
{
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Quitting game"));
	
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);
	}
}

void AMainMenuGameMode::OpenSettings()
{
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Opening settings (TODO: Implement settings menu)"));
	// TODO: Create and show settings widget
}

void AMainMenuGameMode::OpenStore()
{
	UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Opening gem store (TODO: Implement store)"));
	// TODO: Create and show store widget
}