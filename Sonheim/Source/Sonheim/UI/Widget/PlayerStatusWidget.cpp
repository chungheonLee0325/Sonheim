#include "PlayerStatusWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UPlayerStatusWidget::UpdateHealth(float CurrentHP, float Delta, float MaxHP)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(CurrentHP / MaxHP);
    }
    
    if (HealthText)
    {
        HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), CurrentHP, MaxHP)));
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

void UPlayerStatusWidget::UpdateHPPotionStack(int stack)
{
    if (HPPotionStack)
    {
        HPPotionStack->SetText(FText::FromString(FString::Printf(TEXT("%d"), stack)));
    }
}

void UPlayerStatusWidget::DisableHPPotionBG(bool IsLoop)
{
    if (IsLoop)
    {
        HPPotionBG->SetColorAndOpacity(FLinearColor(1,1,1,1));
    }
    else
    {
        PlayAnimation(DisableAnimation);
    }
}
