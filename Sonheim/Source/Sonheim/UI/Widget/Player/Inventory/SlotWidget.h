// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/CustomWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "SlotWidget.generated.h"

class UToolTipWidget;
class UDragDropSlotOperation;

/**
 * 
 */
UCLASS()
class SONHEIM_API USlotWidget : public UCustomWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
public:
	// 슬롯 초기화 함수
	void Init(int Index);
	// 슬롯을 업데이트해주기 위한 함수
	void SetItemData(const FItemData* ItemData, int32 NewQuantity);
	void ClearSlot();

public:
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

	// 슬롯 타입 - 인벤토리인지 장비창인지 구분
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	EEquipmentSlotType EquipmentSlotType = EEquipmentSlotType::None;
	
	// 툴팁 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	TSubclassOf<UToolTipWidget> ToolTipWidgetClass;
	
	// 드래그 드롭 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	bool bSupportsDragDrop = true;
	
	// 아이템 클릭 이벤트
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemClicked, USlotWidget*, SlotWidget, bool, bIsRightClick);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemClicked OnItemClicked;
	
	// 아이템 드래그 시작 이벤트
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDragStarted, USlotWidget*, SlotWidget);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemDragStarted OnItemDragStarted;
	
	// 아이템 드롭 이벤트
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDropped, USlotWidget*, FromSlot, USlotWidget*, ToSlot);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemDropped OnItemDropped;
	
private:
	UPROPERTY()
	UToolTipWidget* ToolTipInstance;
	UPROPERTY()
	USonheimGameInstance* m_GameInstance;

protected:
	bool IsEmpty() const;
    
	// 빈 칸에 적용하기 위한 투명 텍스쳐
	UPROPERTY(EditAnywhere, Category = "Slot")
	TObjectPtr<class UTexture2D> DefaultTexture;
};

// 드래그 드롭 오퍼레이션 클래스
UCLASS()
class SONHEIM_API UDragDropSlotOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	USlotWidget* DraggedSlotWidget;
};
