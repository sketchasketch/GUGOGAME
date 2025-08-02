#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameCountdown.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);

UCLASS()
class GU_GO_API UGameCountdown : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StartCountdown(float Duration = 3.0f);

	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StopCountdown();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCountdownFinished OnCountdownFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown")
	float CountdownDuration = 3.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CountdownText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ReadyText;

	UPROPERTY(meta = (BindWidget))
	class UImage* CountdownBackground;

private:
	float CurrentCountdownTime = 0.0f;
	bool bIsCountingDown = false;

	void UpdateCountdownDisplay();
	FString GetCountdownDisplayText(float TimeRemaining) const;
};