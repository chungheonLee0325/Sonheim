// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterStatusWidget.h"

#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"

void UMonsterStatusWidget::InitMonsterStatusWidget(FAreaObjectData* AreaObjectData, bool Gender, int Level)
{
	// Set Name
	this->MonsterNameText->SetText(FText::FromName(AreaObjectData->Name));

	// Set Level
	this->LevelText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Level)));

	// Set Gender
	FString gender = "♂♀";
	gender = Gender ? "♂" : "♀";
	this->GenderText->SetText(FText::FromString(gender));

	// Set Element Attribute Type
	TArray<EElementalAttribute> elementArray = AreaObjectData->DefenceElementalAttributes.Array();

	if (elementArray.Num() == 0)
	{
		TypeSingleOverlay->SetVisibility(ESlateVisibility::Hidden);
		TypeMultiOverlay->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	else if (elementArray.Num() == 1)
	{
		TypeMultiOverlay->SetVisibility(ESlateVisibility::Hidden);

		FLinearColor color = CalcImageByElementAttributes(elementArray[0]);
		TypeSingleImage->SetColorAndOpacity(color);
	}
	else
	{
		TypeSingleOverlay->SetVisibility(ESlateVisibility::Hidden);

		FLinearColor color = CalcImageByElementAttributes(elementArray[0]);
		TypeOneImage->SetColorAndOpacity(color);
		FLinearColor color2 = CalcImageByElementAttributes(elementArray[1]);
		TypeTwoImage->SetColorAndOpacity(color2);
	}
}

FLinearColor UMonsterStatusWidget::CalcImageByElementAttributes(EElementalAttribute Type)
{
	switch (Type)
	{
	case (EElementalAttribute::Dark):
		{
			return FLinearColor(0.52f, 0.f, 0.f);
		}
	case (EElementalAttribute::Dragon):
		{
			return FLinearColor(0.5f, 0.f, 0.5f);
		}
	case (EElementalAttribute::Electric):
		{
			return FLinearColor(1.0f, 1.0f, 0.0f);
		}
	case (EElementalAttribute::Fire):
		{
			return FLinearColor(1.0f, 0.f, 0.f);
		}
	case (EElementalAttribute::Grass):
		{
			return FLinearColor(0.f, 1.f, 0.f);
		}
	case (EElementalAttribute::Ground):
		{
			return FLinearColor(0.5882f, 0.2941f, 0.f);
		}
	case (EElementalAttribute::Ice):
		{
			return FLinearColor(0.3137f, 0.7373f, 0.8745f);
		}
	case (EElementalAttribute::Neutral):
		{
			return FLinearColor(0.9608f, 0.9608f, 0.8627f);
		}
	case (EElementalAttribute::Water):
		{
			return FLinearColor(0.f, 0.f, 1.0f);
		}
	}
	return FLinearColor(0.f, 0.f, 0.f);
}
