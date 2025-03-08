#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    
public:
    UFUNCTION()
    void UpdateHealth(float CurrentHP, float Delta, float MaxHP);
    UFUNCTION()
    void UpdateStamina(float CurrentStamina, float Delta, float MaxStamina);
    UFUNCTION()
    void UpdateHPPotionStack(int stack);
    UFUNCTION()
    void DisableHPPotionBG(bool IsLoop);

protected:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;
    
    UPROPERTY(meta = (BindWidget))
    UProgressBar* StaminaBar;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthText;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StaminaText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HPPotionStack;

    UPROPERTY(meta = (BindWidget))
    UImage* HPPotionBG;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* DisableAnimation;
}; 