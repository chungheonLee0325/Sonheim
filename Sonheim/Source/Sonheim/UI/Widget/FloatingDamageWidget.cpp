#include "FloatingDamageWidget.h"
#include "Components/TextBlock.h"

bool UFloatingDamageWidget::Initialize()
{
	// 색 초기화
	DamageWeakPointColors.Add(EFloatingOutLineDamageType::CriticalDamaged, (FLinearColor(1.0f, 0.0f, 0.0f, 1.0f)));
	DamageWeakPointColors.Add(EFloatingOutLineDamageType::WeakPointDamage, (FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)));
	DamageWeakPointColors.Add(EFloatingOutLineDamageType::Normal, (FLinearColor(0, 0, 0, 1.0f)));
	
	DamageElementAttributeColors.Add(EFloatingTextDamageType::EffectiveElementDamage, (FLinearColor(1.0f, 0.0f, 0.0f, 1.0f)));
	DamageElementAttributeColors.Add(EFloatingTextDamageType::InefficientElementDamage, (FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)));
	DamageElementAttributeColors.Add(EFloatingTextDamageType::Normal, (FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	return Super::Initialize();
}


void UFloatingDamageWidget::SetDamageInfo(float Damage, EFloatingOutLineDamageType WeakPointType,  EFloatingTextDamageType ElementAttributeType)
{
	if (!DamageText) return;

	// 데미지 텍스트 설정
	FText DamageString = FText::FromString(FString::Printf(TEXT("%.0f"), FMath::Abs(Damage)));
	DamageText->SetText(DamageString);

	// 데미지 타입에 따른 색상 설정
	if (const FLinearColor* Color = DamageElementAttributeColors.Find(ElementAttributeType))
	{
		DamageText->SetColorAndOpacity(FSlateColor(*Color));
	}
	else
	{
		DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))); // 흰색
	}

	// 데미지 타입에 따른 색상 설정
	if (const FLinearColor* Color = DamageWeakPointColors.Find(WeakPointType))
	{
		DamageText->Font.OutlineSettings.OutlineColor = *Color;
	}
	else
	{
		DamageText->Font.OutlineSettings.OutlineColor = FLinearColor(.0f, .0f, .0f, 1.0f); // 흰색
	}
}

void UFloatingDamageWidget::PlayFadeAnimation()
{
	if (FadeOutAnimation)
	{
		PlayAnimation(FadeOutAnimation);
	}
}
