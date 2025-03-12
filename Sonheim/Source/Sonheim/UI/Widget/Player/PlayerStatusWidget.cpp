#include "PlayerStatusWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPlayerStatusWidget::UpdateLevel(int32 OldLevel, int32 NewLevel, bool bLevelUp)
{
	if (LevelText && bLevelUp)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("%2d"), NewLevel)));
	}
}

void UPlayerStatusWidget::UpdateExp(int32 CurrentExp, int32 MaxExp, int32 Delta)
{
	if (ExpBar && ExpText)
	{
		ExpBar->SetPercent((float)CurrentExp / MaxExp);
		ExpText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Delta)));
	}
}

void UPlayerStatusWidget::UpdateStamina(float CurrentStamina, float Delta, float MaxStamina)
{
	if (StaminaBar)
	{
		StaminaBar->SetPercent(CurrentStamina / MaxStamina);
	}

	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), CurrentStamina, MaxStamina)));
	}
}
