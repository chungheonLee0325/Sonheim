#include "DetectWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void UDetectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 초기 상태 설정
	if (HoldProgressBar)
	{
		HoldProgressBar->SetPercent(0.0f);
	}
}

void UDetectWidget::SetInteractionInfo(const FString& ItemName, const FString& InteractName, const FString& KeyName)
{
	if (NameText)
	{
		NameText->SetText(FText::FromString(ItemName));
	}

	if (InteractText)
	{
		InteractText->SetText(FText::FromString(InteractName));
	}
	
	if (KeyText)
	{
		FString FormattedKey = FString::Printf(TEXT("%s"), *KeyName);
		KeyText->SetText(FText::FromString(FormattedKey));
	}
}

void UDetectWidget::UpdateHoldProgress(float Progress)
{
	if (HoldProgressBar)
	{
		HoldProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}
}

void UDetectWidget::UpdateDistanceFade(float Distance, float MaxDistance)
{
	// 거리에 따른 투명도 계산 (가까울수록 선명)
	float Alpha = 1.0f - (Distance / MaxDistance);
	Alpha = FMath::Clamp(Alpha * 2.0f, 0.7f, 1.0f); // 최소 70% 투명도
	
	SetRenderOpacity(Alpha);
}