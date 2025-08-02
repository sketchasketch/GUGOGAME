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
	void ShowGameOver();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideGameOver();

protected:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* ScoreText;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UTextBlock* DistanceText;

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
};