// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/UI/Widget/CustomWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "ToolTipWidget.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UToolTipWidget : public UCustomWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	// 툴팁 초기화 함수
	void InitToolTip(const FItemData* ItemData, int32 Quantity);
	
protected:
	// 아이템 아이콘
	UPROPERTY(meta = (BindWidget))
	class UImage* IMG_ItemIcon;
	
	// 아이템 이름
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_ItemName;
	
	// 아이템 설명
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_ItemDescription;
	
	// 아이템 무게
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_ItemWeight;
	
	// 아이템 수량
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_ItemQuantity;
	
	// 아이템 스탯 정보 (장비인 경우)
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* VB_Stats;
	
	// 장비 정보 스탯들 (HP, 방어력 등)
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_HPBonus;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_DefenseBonus;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_DamageBonus;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TXT_StaminaBonus;
};
