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

protected:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* ScoreText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* DistanceText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* CoinsText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* StepsText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* PauseButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UUserWidget* PauseMenuWidget;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UVerticalBox* GameOverPanel;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* FinalScoreText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* RestartButton;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UButton* MainMenuButton;

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnPauseClicked();
};