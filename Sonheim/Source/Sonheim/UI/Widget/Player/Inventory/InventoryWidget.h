// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Widget/CustomWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "InventoryWidget.generated.h"

class USlotWidget;
class UUniformGridPanel;
class UInventoryComponent;
/**
 * 
 */
UCLASS()
class SONHEIM_API UInventoryWidget : public UCustomWidget

{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
    
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* SlotGrid;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	int32 GridColumns = 6;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	int32 GridRows = 7;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
	TSubclassOf<USlotWidget> SlotWidgetClass;
    
	UPROPERTY()
	TArray<USlotWidget*> SlotWidgets;

	// Equipment Slot
	UPROPERTY(meta = (BindWidget))
	USlotWidget* HeadSlot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* BodySlot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Weapon1Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Weapon2Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Weapon3Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Weapon4Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Accessory1Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* Accessory2Slot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* ShieldSlot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* GliderSlot;
	UPROPERTY(meta = (BindWidget))
	USlotWidget* SphereModuleSlot;

	// 인벤토리 컴포넌트 참조
	UPROPERTY(BlueprintReadWrite)
	UInventoryComponent* InventoryComponent;

public:
	UFUNCTION()
	void UpdateInventoryFromData(const TArray<FInventoryItem>& InventoryData);
	UFUNCTION()
	void UpdateEquipmentFromData(EEquipmentSlotType EquipSlot, FInventoryItem InventoryItem);
	
	// 인벤토리 컴포넌트 설정
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryComponent(UInventoryComponent* InInventoryComponent);
	
	// 슬롯 클릭 이벤트 핸들러
	UFUNCTION()
	void OnSlotClicked(USlotWidget* SlotWidget, bool bIsRightClick);
	
	// 슬롯 드래그 시작 이벤트 핸들러
	UFUNCTION()
	void OnSlotDragStarted(USlotWidget* SlotWidget);
	
	// 슬롯 드롭 이벤트 핸들러
	UFUNCTION()
	void OnSlotDropped(USlotWidget* FromSlot, USlotWidget* ToSlot);
	
private:
	void InitializeSlotWidgetMap();
	
	UPROPERTY()
	USonheimGameInstance* m_GameInstance;

	UPROPERTY()
	TMap<EEquipmentSlotType, USlotWidget*> SlotWidgetMap;

	// 슬롯 위젯에 이벤트 처리기 연결
	void BindSlotEvents(USlotWidget* SlotWidget);
	
	// 인벤토리 슬롯 아이템 교환
	void SwapInventoryItems(int32 FromIndex, int32 ToIndex);
	
	// 장비 슬롯 아이템 처리
	void HandleEquipmentSlotInteraction(USlotWidget* SlotWidget, bool bIsRightClick);
	
	// 인벤토리 슬롯 아이템 처리
	void HandleInventorySlotInteraction(USlotWidget* SlotWidget, bool bIsRightClick);
};
