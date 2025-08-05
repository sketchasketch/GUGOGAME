#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunnerHUD.generated.h"

UCLASS()
class GU_GO_API URunnerHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateScore(int32 NewScore);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateDistance(float NewDistance);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateCoins(int32 NewCoins);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateSteps(int32 NewSteps);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HidePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowGameOver();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideGameOver();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowContinuePrompt(int32 GemsOwned, int32 ContinueCost);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideContinuePrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateGems(int32 NewGems);

protected:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* ScoreText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* DistanceText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* PlayerCoins;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* PlayerSteps;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* PlayerGems;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* GamePauseBtn;

	// Pause Menu UI Elements
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UVerticalBox* PauseMenuPanel;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* ResumeButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* RestartFromPauseButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* MainMenuFromPauseButton;

	// Game Over UI Elements
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UVerticalBox* GameOverPanel;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* FinalScoreText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* RestartButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* MainMenuButton;

	// Continue Prompt UI Elements
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UVerticalBox* ContinuePromptPanel;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* ContinuePromptText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* GemsInfoText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* ContinueButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* DeclineButton;

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnPauseClicked();

	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnDeclineClicked();

	// Pause menu button handlers
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartFromPauseClicked();

	UFUNCTION()
	void OnMainMenuFromPauseClicked();
};