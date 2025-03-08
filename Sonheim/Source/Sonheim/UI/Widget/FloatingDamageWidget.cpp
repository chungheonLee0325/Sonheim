#include "FloatingDamageWidget.h"
#include "Components/TextBlock.h"

void UFloatingDamageWidget::SetDamageInfo(float Damage, EFloatingDamageType Type)
{
    if (!DamageText) return;

    // 데미지 텍스트 설정
    FText DamageString = FText::FromString(FString::Printf(TEXT("%.0f"), FMath::Abs(Damage)));
    DamageText->SetText(DamageString);

    // 데미지 타입에 따른 색상 설정
    if (const FLinearColor* Color = DamageColors.Find(Type))
    {
        DamageText->SetColorAndOpacity(FSlateColor(*Color));
    }
    else
    {
        // 기본 색상 설정
        switch (Type)
        {
        case EFloatingDamageType::PlayerDamaged:
            DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f))); // 빨간색
            break;
        case EFloatingDamageType::WeakPointDamage:
            DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f))); // 초록색
            break;
        default:
            DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))); // 흰색
            break;
        }
    }
}

void UFloatingDamageWidget::PlayFadeAnimation()
{
    if (FadeOutAnimation)
    {
        PlayAnimation(FadeOutAnimation);
    }
} 