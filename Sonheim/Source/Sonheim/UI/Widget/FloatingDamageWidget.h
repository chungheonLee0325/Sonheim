#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingDamageWidget.generated.h"

UENUM(BlueprintType)
enum class EFloatingOutLineDamageType : uint8
{
    Normal,
    WeakPointDamage,
    CriticalDamaged,
};
enum class EFloatingTextDamageType : uint8
{
    Normal,
    InefficientElementDamage,
    EffectiveElementDamage,
};


UCLASS()
class SONHEIM_API UFloatingDamageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetDamageInfo(float Damage, EFloatingOutLineDamageType WeakPointType,  EFloatingTextDamageType ElementAttributeType);
    void PlayFadeAnimation();

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* DamageText;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* FadeOutAnimation;

    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TMap<EFloatingOutLineDamageType, FLinearColor> DamageWeakPointColors;
    TMap<EFloatingTextDamageType, FLinearColor> DamageElementAttributeColors;
}; 