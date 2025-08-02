#include "GameCountdown.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UGameCountdown::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Hide initially
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGameCountdown::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bIsCountingDown)
	{
		CurrentCountdownTime -= InDeltaTime;
		
		if (CurrentCountdownTime <= 0.0f)
		{
			// Countdown finished
			bIsCountingDown = false;
			SetVisibility(ESlateVisibility::Collapsed);
			OnCountdownFinished.Broadcast();
		}
		else
		{
			UpdateCountdownDisplay();
		}
	}
}

void UGameCountdown::StartCountdown(float Duration)
{
	CountdownDuration = Duration;
	CurrentCountdownTime = CountdownDuration;
	bIsCountingDown = true;
	
	SetVisibility(ESlateVisibility::Visible);
	UpdateCountdownDisplay();
}

void UGameCountdown::StopCountdown()
{
	bIsCountingDown = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGameCountdown::UpdateCountdownDisplay()
{
	if (CountdownText && ReadyText)
	{
		if (CurrentCountdownTime > 1.0f)
		{
			// Show countdown number
			CountdownText->SetVisibility(ESlateVisibility::Visible);
			ReadyText->SetVisibility(ESlateVisibility::Collapsed);
			CountdownText->SetText(FText::FromString(GetCountdownDisplayText(CurrentCountdownTime)));
		}
		else
		{
			// Show "GO!" message
			CountdownText->SetVisibility(ESlateVisibility::Collapsed);
			ReadyText->SetVisibility(ESlateVisibility::Visible);
			ReadyText->SetText(FText::FromString("GO!"));
		}
	}
}

FString UGameCountdown::GetCountdownDisplayText(float TimeRemaining) const
{
	int32 CountdownNumber = FMath::CeilToInt(TimeRemaining);
	
	switch (CountdownNumber)
	{
		case 3: return "3";
		case 2: return "2";
		case 1: return "1";
		default: return "GO!";
	}
}