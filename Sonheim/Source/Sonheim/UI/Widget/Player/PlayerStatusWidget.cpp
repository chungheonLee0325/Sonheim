#include "PlayerStatusWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"

void UPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PalSlots.Add(0, PalSlot0);
	PalSlots.Add(1, PalSlot1);
	PalSlots.Add(2, PalSlot2);
	PalSlots.Add(3, PalSlot3);
	PalSlots.Add(4, PalSlot4);

	SelectBGs.Add(0, SelectBG0);
	SelectBGs.Add(1, SelectBG1);
	SelectBGs.Add(2, SelectBG2);
	SelectBGs.Add(3, SelectBG3);
	SelectBGs.Add(4, SelectBG4);

	for (auto bg : SelectBGs)
	{
		bg.Value->SetVisibility(ESlateVisibility::Hidden);
	}
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

void UPlayerStatusWidget::SetEnableCrossHair(bool IsActive)
{
	ESlateVisibility bShow = IsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (IsActive) PlayAnimation(ZoomInByLockOn);
	CrossHair->SetVisibility(bShow);
}

void UPlayerStatusWidget::AddOwnedPal(int MonsterID, int Index)
{
	USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (gameInstance == nullptr)
	{
		return;
	}
	gameInstance->GetDataAreaObject(MonsterID);

	PalSlots[Index]->SetBrushFromTexture(gameInstance->GetDataAreaObject(MonsterID)->AreaObjectIcon);
	PalSlots[Index]->SetRenderOpacity(1.0f);
}

void UPlayerStatusWidget::SwitchSelectedPalIndex(int Index)
{
	for (auto bg : SelectBGs)
	{
		if (bg.Key == Index)
		{
			bg.Value->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			bg.Value->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
