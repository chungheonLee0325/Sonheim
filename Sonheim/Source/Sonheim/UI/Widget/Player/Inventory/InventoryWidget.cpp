// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"

#include "SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"

void UInventoryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	// 그리드 패널이 존재하고 슬롯 위젯 클래스가 지정되었는지 확인
	if (SlotGrid && SlotWidgetClass)
	{
		// 기존 슬롯 위젯 초기화
		SlotGrid->ClearChildren();
		SlotWidgets.Empty();

		// 그리드 크기에 맞춰 슬롯 위젯 생성
		for (int32 Row = 0; Row < GridRows; Row++)
		{
			for (int32 Column = 0; Column < GridColumns; Column++)
			{
				USlotWidget* SlotWidget = CreateWidget<USlotWidget>(this, SlotWidgetClass);
				if (SlotWidget)
				{
					// 그리드에 슬롯 추가
					SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Column);
					SlotWidgets.Add(SlotWidget);

					// 슬롯 초기화
					SlotWidget->ClearSlot();
					SlotWidget->Init(SlotWidgets.Num() - 1);
					//SlotWidget->Init(SlotWidgets.IndexOfByKey(SlotWidget));
					//SlotWidget->Init(Row * Column + Row);
				}
			}
		}
	}
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 데이터 초기화
	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	InitializeSlotWidgetMap();
}

void UInventoryWidget::UpdateInventoryFromData(const TArray<FInventoryItem>& InventoryData)
{
	// 슬롯 위젯 배열이 비어있지 않은지 확인
	if (SlotWidgets.Num() > 0)
	{
		// 모든 슬롯 초기화
		for (USlotWidget* SlotWidget : SlotWidgets)
		{
			if (SlotWidget)
			{
				SlotWidget->ClearSlot();
			}
		}

		// 인벤토리 데이터로 슬롯 업데이트
		for (int32 i = 0; i < InventoryData.Num() && i < SlotWidgets.Num(); i++)
		{
			const FItemData* ItemData = m_GameInstance->GetDataItem(InventoryData[i].ItemID);
			if (ItemData == nullptr)
			{
				return;
			}

			SlotWidgets[i]->SetItemData(ItemData, InventoryData[i].Count);
		}
	}
}

void UInventoryWidget::UpdateEquipmentFromData(EEquipmentSlotType EquipSlot, FInventoryItem InventoryItem)
{
	const FItemData* ItemData = m_GameInstance->GetDataItem(InventoryItem.ItemID);
	
	if (EquipSlot == EEquipmentSlotType::None || EquipSlot == EEquipmentSlotType::Max)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid equipment slot type: %d"), (int32)EquipSlot);
		return;
	}
    
	USlotWidget** SlotPtr = SlotWidgetMap.Find(EquipSlot);
	if (SlotPtr && *SlotPtr)
	{
		if (ItemData == nullptr)
		{
			(*SlotPtr)->ClearSlot();
		}
		else
		{
			(*SlotPtr)->SetItemData(ItemData, InventoryItem.Count);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Slot widget not found for type: %d"), (int32)EquipSlot);
	}
}

void UInventoryWidget::InitializeSlotWidgetMap()
{
	SlotWidgetMap.Add(EEquipmentSlotType::Head, HeadSlot);
	SlotWidgetMap.Add(EEquipmentSlotType::Body, BodySlot);
	SlotWidgetMap.Add(EEquipmentSlotType::Weapon1, Weapon1Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Weapon2, Weapon2Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Weapon3, Weapon3Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Weapon4, Weapon4Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Accessory1, Accessory1Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Accessory2, Accessory2Slot);
	SlotWidgetMap.Add(EEquipmentSlotType::Shield, ShieldSlot);
	SlotWidgetMap.Add(EEquipmentSlotType::Glider, GliderSlot);
	SlotWidgetMap.Add(EEquipmentSlotType::SphereModule, SphereModuleSlot);
}
