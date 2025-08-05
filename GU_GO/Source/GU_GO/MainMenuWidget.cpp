#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Error, TEXT("***** MAIN MENU WIDGET NATIVE CONSTRUCT *****"));
	UE_LOG(LogTemp, Error, TEXT("PlayButton: %s"), PlayButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("QuitButton: %s"), QuitButton ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("TitleText: %s"), TitleText ? TEXT("EXISTS") : TEXT("NULL"));
	UE_LOG(LogTemp, Error, TEXT("HighScoreText: %s"), HighScoreText ? TEXT("EXISTS") : TEXT("NULL"));

	// Bind button events
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayButtonClicked);
		UE_LOG(LogTemp, Warning, TEXT("PlayButton bound successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayButton is NULL - cannot bind click event!"));
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitButtonClicked);
		UE_LOG(LogTemp, Warning, TEXT("QuitButton bound successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("QuitButton is NULL - cannot bind click event!"));
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
	UE_LOG(LogTemp, Error, TEXT("***** PLAY BUTTON CLICKED *****"));
	
	// Get game mode and start game
	if (AMainMenuGameMode* GameMode = Cast<AMainMenuGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		UE_LOG(LogTemp, Warning, TEXT("MAIN_MENU: Found MainMenuGameMode, starting game"));
		
		// Remove this widget
		RemoveFromParent();
		
		// Start the game
		GameMode->StartGame();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MAIN_MENU: Could not cast to MainMenuGameMode!"));
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