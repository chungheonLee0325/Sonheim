// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/CustomWidget.h"
#include "SlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USlotWidget : public UCustomWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	
public:
	// 슬롯 초기화 함수
	void Init(int Index);
	// ToDo: 고민해보자..
	// 슬롯의 타입을 지정해주기 위한 함수
	//void SetType(ESlotType Type);
	// 슬롯을 업데이트해주기 위한 함수
	void UpdateSlot();
	void NativePreConstruct();
	void SetItemData(const FItemData* ItemData, int32 NewQuantity);
	void ClearSlot();

public:
	// ToDo: 고민해보자..
	// 현재 슬롯의 타입
	//UPROPERTY(VisibleAnywhere, Category = "Slot")
	//ESlotType SlotType;

	// 슬롯에 지정될 이미지
	UPROPERTY(VisibleAnywhere, Category = "Slot", meta = (BindWidget = "true"))
	TObjectPtr<class UImage> IMG_Item;
	
	// 슬롯에 지정될 배경
	UPROPERTY(VisibleAnywhere, Category = "Slot", meta = (BindWidget = "true"))
	TObjectPtr<class UImage> IMG_BG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	UTexture2D* IMG_Default;

	// 슬롯에 지정될 아이템의 수량
	UPROPERTY(VisibleAnywhere, Category = "Slot", meta = (BindWidget = "true"))
	TObjectPtr<class UTextBlock> TXT_Quantity;

	// 슬롯에 지정될 아이템의 무게
	UPROPERTY(VisibleAnywhere, Category = "Slot", meta = (BindWidget = "true"))
	TObjectPtr<class UTextBlock> TXT_Weight;

	// 현재 슬롯의 인덱스
	UPROPERTY(EditAnywhere, Category = "Slot")
	int32 SlotIndex;
	UPROPERTY(EditAnywhere, Category = "Slot")
	int ItemID;
	int Quantity;

protected:
	bool IsEmpty() const;
    
	// 빈 칸에 적용하기 위한 투명 텍스쳐
	UPROPERTY(EditAnywhere, Category = "Slot")
	TObjectPtr<class UTexture2D> DefaultTexture;
};
