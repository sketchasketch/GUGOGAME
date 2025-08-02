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
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ScoreText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DistanceText;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* GameOverPanel;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* FinalScoreText;

	UPROPERTY(meta = (BindWidget))
	class UButton* RestartButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* MainMenuButton;

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();
};