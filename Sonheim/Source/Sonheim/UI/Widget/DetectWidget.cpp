#include "DetectWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Sonheim/GameObject/InteractableInterface.h"

void UDetectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 상태 설정
	if (InteractProgressBar)
	{
		InteractProgressBar->SetPercent(0.0f);
	}
	if (CancelProgressBar)
	{
		CancelProgressBar->SetPercent(0.f);
	}
	if (CancelRow)
	{
		CancelRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDetectWidget::SetInteractionInfo(const FString& ItemName, const FString& InteractName, const FString& KeyName)
{
	if (InteractNameText)
	{
		InteractNameText->SetText(FText::FromString(ItemName));
	}

	if (InteractText)
	{
		InteractText->SetText(FText::FromString(InteractName));
	}

	if (InteractKeyText)
	{
		FString FormattedKey = FString::Printf(TEXT("%s"), *KeyName);
		InteractKeyText->SetText(FText::FromString(FormattedKey));
	}
}

void UDetectWidget::UpdateInteractProgress(float Progress)
{
	if (InteractProgressBar)
	{
		InteractProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}
}

void UDetectWidget::UpdateDistanceFade(float Distance, float MaxDistance)
{
	// 거리에 따른 투명도 계산 (가까울수록 선명)
	float Alpha = 1.0f - (Distance / MaxDistance);
	Alpha = FMath::Clamp(Alpha * 2.0f, 0.7f, 1.0f); // 최소 70% 투명도

	SetRenderOpacity(Alpha);
}

void UDetectWidget::SetCancelVisible(bool bVisible)
{
	if (CancelRow)
	{
		CancelRow->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bVisible && CancelProgressBar)
	{
		CancelProgressBar->SetPercent(0.f);
	}
}

void UDetectWidget::SetCancelInfo(const FText& ActionText, const FText& KeyText)
{
	if (CancelActionText) CancelActionText->SetText(ActionText);
	if (CancelKeyText) CancelKeyText->SetText(KeyText);
}

void UDetectWidget::UpdateCancelHoldProgress(float Progress)
{
	if (CancelProgressBar)
	{
		CancelProgressBar->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
	}
}

void UDetectWidget::UpdateHoldProgressByPurpose(float Progress, EHoldPurpose Purpose)
{
	if (Purpose == EHoldPurpose::Cancel) UpdateCancelHoldProgress(Progress);
	else UpdateInteractProgress(Progress);
}
