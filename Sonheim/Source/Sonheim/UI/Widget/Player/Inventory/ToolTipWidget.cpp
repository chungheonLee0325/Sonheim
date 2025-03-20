// Fill out your copyright notice in the Description page of Project Settings.


#include "ToolTipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Sonheim/Utilities/SonheimUtility.h"

void UToolTipWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UToolTipWidget::InitToolTip(const FItemData* ItemData, int32 Quantity)
{
	if (!ItemData)
		return;

	// 아이템 아이콘 설정
	if (IMG_ItemIcon)
	{
		IMG_ItemIcon->SetBrushFromTexture(ItemData->ItemIcon);
	}

	// 아이템 이름 설정
	if (TXT_ItemName)
	{
		TXT_ItemName->SetText(ItemData->ItemName);
	}

	// 아이템 설명 설정
	if (TXT_ItemDescription)
	{
		TXT_ItemDescription->SetText(USonheimUtility::ConvertEscapedNewlinesToFText(ItemData->ItemDescription));
		//TXT_ItemDescription->SetText(ItemData->ItemDescription);
	}

	// 아이템 무게 설정
	if (TXT_ItemWeight)
	{
		TXT_ItemWeight->SetText(FText::FromString(FString::Printf(TEXT("무게 : %.1f"), ItemData->Weight)));
	}

	// 아이템 수량 설정
	if (TXT_ItemQuantity)
	{
		TXT_ItemQuantity->SetText(FText::FromString(FString::Printf(TEXT("보유 수 : %d"), Quantity)));
	}

	// 장비인 경우 스탯 정보 표시
	if (ItemData->ItemCategory == EItemCategory::Equipment ||
		ItemData->ItemCategory == EItemCategory::Weapon)
	{
		if (VB_Stats)
		{
			VB_Stats->SetVisibility(ESlateVisibility::Visible);

			// HP 보너스
			if (TXT_HPBonus && ItemData->EquipmentData.HPBonus != 0)
			{
				TXT_HPBonus->SetText(
					FText::FromString(FString::Printf(TEXT("체력 : +%.0f"), ItemData->EquipmentData.HPBonus)));
				TXT_HPBonus->SetVisibility(ESlateVisibility::Visible);
			}
			else if (TXT_HPBonus)
			{
				TXT_HPBonus->SetVisibility(ESlateVisibility::Collapsed);
			}

			// 방어력 보너스
			if (TXT_DefenseBonus && ItemData->EquipmentData.DefenseBonus != 0)
			{
				TXT_DefenseBonus->SetText(
					FText::FromString(FString::Printf(TEXT("방어력 : +%.0f"), ItemData->EquipmentData.DefenseBonus)));
				TXT_DefenseBonus->SetVisibility(ESlateVisibility::Visible);
			}
			else if (TXT_DefenseBonus)
			{
				TXT_DefenseBonus->SetVisibility(ESlateVisibility::Collapsed);
			}

			// 공격력 보너스
			if (TXT_DamageBonus && ItemData->EquipmentData.DamageBonus != 0)
			{
				TXT_DamageBonus->SetText(
					FText::FromString(FString::Printf(TEXT("공격력 : +%.0f"), ItemData->EquipmentData.DamageBonus)));
				TXT_DamageBonus->SetVisibility(ESlateVisibility::Visible);
			}
			else if (TXT_DamageBonus)
			{
				TXT_DamageBonus->SetVisibility(ESlateVisibility::Collapsed);
			}

			// 스태미나 보너스
			if (TXT_StaminaBonus && ItemData->EquipmentData.StaminaBonus != 0)
			{
				TXT_StaminaBonus->SetText(
					FText::FromString(FString::Printf(TEXT("스태미나 : +%.0f"), ItemData->EquipmentData.StaminaBonus)));
				TXT_StaminaBonus->SetVisibility(ESlateVisibility::Visible);
			}
			else if (TXT_StaminaBonus)
			{
				TXT_StaminaBonus->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	else if (VB_Stats)
	{
		VB_Stats->SetVisibility(ESlateVisibility::Collapsed);
	}
}
