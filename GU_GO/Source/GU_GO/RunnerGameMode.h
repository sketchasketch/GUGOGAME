#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RunnerGameMode.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
	MainMenu    UMETA(DisplayName = "Main Menu"),
	Playing     UMETA(DisplayName = "Playing"),
	Paused      UMETA(DisplayName = "Paused"),
	GameOver    UMETA(DisplayName = "Game Over")
};

UCLASS()
class GU_GO_API ARunnerGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ARunnerGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Points);

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintCallable, Category = "Score")
	float GetDistanceRun() const { return DistanceRun; }

	UFUNCTION(BlueprintCallable, Category = "Game")
	void GameOver();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void PauseGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Game")
	EGameState GetCurrentGameState() const { return CurrentGameState; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float InitialGameSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float SpeedIncreaseRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float MaxGameSpeed = 2000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	float CurrentGameSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bIsGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	EGameState CurrentGameState = EGameState::MainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class URunnerHUD> HUDWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

protected:
	UPROPERTY()
	class ARunnerCharacter* PlayerCharacter;

	UPROPERTY()
	class URunnerHUD* RunnerHUD;

private:
	int32 Score = 0;
	float DistanceRun = 0.0f;
	float TimePlayed = 0.0f;
};