#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingDamageWidget.generated.h"

UENUM(BlueprintType)
enum class EFloatingOutLineDamageType : uint8
{
    Normal         UMETA(DisplayName = "Normal"),
    WeakPointDamage UMETA(DisplayName = "Weak Point"),
    CriticalDamaged UMETA(DisplayName = "Critical"),
};

UENUM(BlueprintType)
enum class EFloatingTextDamageType : uint8
{
    Normal                  UMETA(DisplayName = "Normal"),
    InefficientElementDamage UMETA(DisplayName = "Inefficient Element"),
    EffectiveElementDamage   UMETA(DisplayName = "Effective Element"),
};

UCLASS()
class SONHEIM_API UFloatingDamageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetDamageInfo(float Damage, EFloatingOutLineDamageType WeakPointType, EFloatingTextDamageType ElementAttributeType);
    void PlayFadeAnimation();

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* DamageText;
    
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    UWidgetAnimation* FadeAnimation;

    // 색상 설정
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TMap<EFloatingOutLineDamageType, FLinearColor> DamageWeakPointColors;

    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TMap<EFloatingTextDamageType, FLinearColor> DamageElementAttributeColors;

private:
    void InitializeColorMaps();
};