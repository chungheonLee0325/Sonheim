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

public:
	UFUNCTION()
	void UpdateInventoryFromData(const TArray<FInventoryItem>& InventoryData);
	UFUNCTION()
	void UpdateEquipmentFromData(EEquipmentSlotType EquipSlot, FInventoryItem InventoryItem);
	
private:
	void InitializeSlotWidgetMap();
	
	UPROPERTY()
	USonheimGameInstance* m_GameInstance;

	UPROPERTY()
	TMap<EEquipmentSlotType, USlotWidget*> SlotWidgetMap;
};
