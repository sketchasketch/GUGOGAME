#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class GU_GO_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AMainMenuGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenStore();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
	FName GameLevelName = TEXT("/Game/Maps/InfiniteRunnerLevelWithLandScape");

protected:
	UPROPERTY()
	class UUserWidget* MainMenuWidget;
};