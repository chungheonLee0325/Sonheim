// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseStatusWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBaseStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBaseStatusWidget::UpdateHealth(float CurrentHP, float Delta, float MaxHP)
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
