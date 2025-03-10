#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UBaseStatusWidget
{
    GENERATED_BODY()

protected:
    //virtual void NativeConstruct() override;
    
public:
   // UFUNCTION()
   // void UpdateHealth(float CurrentHP, float Delta, float MaxHP);
   // UFUNCTION()
   // void UpdateStamina(float CurrentStamina, float Delta, float MaxStamina);

protected:
    //UPROPERTY(meta = (BindWidget))
    //UProgressBar* HealthBar;
    //
    //UPROPERTY(meta = (BindWidget))
    //UProgressBar* StaminaBar;
    
    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* HealthText;
    //
    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* StaminaText;
}; 