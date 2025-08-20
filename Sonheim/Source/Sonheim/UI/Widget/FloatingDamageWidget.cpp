#include "FloatingDamageWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

bool UFloatingDamageWidget::Initialize()
{
    bool bResult = Super::Initialize();
    
    if (bResult)
    {
        InitializeColorMaps();
    }
    
    return bResult;
}

void UFloatingDamageWidget::InitializeColorMaps()
{
    // 색상이 에디터에서 설정되지 않았다면 기본값 사용
    if (DamageWeakPointColors.Num() == 0)
    {
        DamageWeakPointColors.Add(EFloatingOutLineDamageType::CriticalDamaged, FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));    // 빨간색
        DamageWeakPointColors.Add(EFloatingOutLineDamageType::WeakPointDamage, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));    // 주황색
        DamageWeakPointColors.Add(EFloatingOutLineDamageType::Normal, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));             // 검은색
    }
    
    if (DamageElementAttributeColors.Num() == 0)
    {
        DamageElementAttributeColors.Add(EFloatingTextDamageType::EffectiveElementDamage, FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));     // 밝은 빨간색
        DamageElementAttributeColors.Add(EFloatingTextDamageType::InefficientElementDamage, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));  // 회색
        DamageElementAttributeColors.Add(EFloatingTextDamageType::Normal, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));                   // 흰색
    }
}

void UFloatingDamageWidget::SetDamageInfo(float Damage, EFloatingOutLineDamageType WeakPointType, EFloatingTextDamageType ElementAttributeType)
{
    if (!DamageText) return;

    // 데미지 텍스트 설정
    FText DamageString = FText::FromString(FString::Printf(TEXT("%.0f"), FMath::Abs(Damage)));
    DamageText->SetText(DamageString);

    // 텍스트 색상 설정
    if (const FLinearColor* Color = DamageElementAttributeColors.Find(ElementAttributeType))
    {
        DamageText->SetColorAndOpacity(FSlateColor(*Color));
    }
    else
    {
        DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    // 외곽선 색상 설정
    FSlateFontInfo FontInfo = DamageText->GetFont();
    
    if (const FLinearColor* OutlineColor = DamageWeakPointColors.Find(WeakPointType))
    {
        FontInfo.OutlineSettings.OutlineColor = *OutlineColor;
    }
    else
    {
        FontInfo.OutlineSettings.OutlineColor = FLinearColor::Black;
    }
    
    DamageText->SetFont(FontInfo);
}

void UFloatingDamageWidget::PlayFadeAnimation()
{
    if (FadeAnimation)
    {
        PlayAnimation(FadeAnimation,0.0f, 1);
    }
}