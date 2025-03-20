// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"

#include "SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"

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
	
	// 모든 슬롯에 이벤트 바인딩
	for (USlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			BindSlotEvents(SlotWidget);
		}
	}
	
	// 장비 슬롯에도 이벤트 바인딩
	for (const TPair<EEquipmentSlotType, USlotWidget*>& Pair : SlotWidgetMap)
	{
		if (Pair.Value)
		{
			Pair.Value->EquipmentSlotType = Pair.Key;
			BindSlotEvents(Pair.Value);
		}
	}
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

void UInventoryWidget::SetInventoryComponent(UInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	
	if (InventoryComponent)
	{
		// 초기 인벤토리 데이터로 UI 업데이트
		UpdateInventoryFromData(InventoryComponent->GetInventory());
		
		// 초기 장비 슬롯 업데이트
		TMap<EEquipmentSlotType, FInventoryItem> EquippedItems = InventoryComponent->GetEquippedItems();
		for (const TPair<EEquipmentSlotType, FInventoryItem>& Pair : EquippedItems)
		{
			UpdateEquipmentFromData(Pair.Key, Pair.Value);
		}
	}
}

void UInventoryWidget::OnSlotClicked(USlotWidget* SlotWidget, bool bIsRightClick)
{
	if (!SlotWidget || !InventoryComponent)
		return;
	
	// 장비 슬롯인 경우
	if (SlotWidget->EquipmentSlotType != EEquipmentSlotType::None)
	{
		HandleEquipmentSlotInteraction(SlotWidget, bIsRightClick);
	}
	// 인벤토리 슬롯인 경우
	else
	{
		HandleInventorySlotInteraction(SlotWidget, bIsRightClick);
	}
}

void UInventoryWidget::OnSlotDragStarted(USlotWidget* SlotWidget)
{
	// 필요한 경우 드래그 시작 시 특별한 처리
}

void UInventoryWidget::OnSlotDropped(USlotWidget* FromSlot, USlotWidget* ToSlot)
{
	if (!FromSlot || !ToSlot || !InventoryComponent)
		return;
	
	// 인벤토리 슬롯 간의 드래그 드롭
	if (FromSlot->EquipmentSlotType == EEquipmentSlotType::None && 
		ToSlot->EquipmentSlotType == EEquipmentSlotType::None)
	{
		// 인벤토리 내에서 아이템 위치 교환
		SwapInventoryItems(FromSlot->SlotIndex, ToSlot->SlotIndex);
	}
	// 인벤토리에서 장비 슬롯으로 드래그 드롭
	else if (FromSlot->EquipmentSlotType == EEquipmentSlotType::None && 
			 ToSlot->EquipmentSlotType != EEquipmentSlotType::None)
	{
		// 장비 장착 시도
		if (InventoryComponent)
		{
			InventoryComponent->EquipItemByIndex(FromSlot->SlotIndex);
		}
	}
	// 장비 슬롯에서 인벤토리로 드래그 드롭
	else if (FromSlot->EquipmentSlotType != EEquipmentSlotType::None && 
			 ToSlot->EquipmentSlotType == EEquipmentSlotType::None)
	{
		// 장비 해제 시도
		if (InventoryComponent)
		{
			InventoryComponent->UnEquipItem(FromSlot->EquipmentSlotType);
		}
	}
	// 장비 슬롯 간의 드래그 드롭 (무시하거나 필요한 경우 특별 처리)
	else if (FromSlot->EquipmentSlotType != EEquipmentSlotType::None && 
			 ToSlot->EquipmentSlotType != EEquipmentSlotType::None)
	{
		return;
		// constexpr int WeaponSlotStartIndex = static_cast<int>(EEquipmentSlotType::Weapon1);
		// constexpr int WeaponSlotEndIndex = static_cast<int>(EEquipmentSlotType::Weapon4);
		//
		// const int FromIndex = static_cast<int>(FromSlot->SlotIndex);
		// const int ToIndex = static_cast<int>(ToSlot->SlotIndex);
		//
		// // 무기 슬롯 범위 확인 함수
		// auto IsValidWeaponSlot = [](int Index) -> bool
		// {
		// 	return Index >= WeaponSlotStartIndex && Index <= WeaponSlotEndIndex;
		// };
		//
		// // 범위 확인 후 아이템 교체
		// if (IsValidWeaponSlot(FromIndex) && IsValidWeaponSlot(ToIndex))
		// {
		// 	// ToDo : 무기 위치 변경 메서드
		// }
	}
}

void UInventoryWidget::BindSlotEvents(USlotWidget* SlotWidget)
{
	if (!SlotWidget)
		return;
	
	// 클릭 이벤트 바인딩
	SlotWidget->OnItemClicked.AddDynamic(this, &UInventoryWidget::OnSlotClicked);
	
	// 드래그 시작 이벤트 바인딩
	SlotWidget->OnItemDragStarted.AddDynamic(this, &UInventoryWidget::OnSlotDragStarted);
	
	// 드롭 이벤트 바인딩
	SlotWidget->OnItemDropped.AddDynamic(this, &UInventoryWidget::OnSlotDropped);
}

void UInventoryWidget::SwapInventoryItems(int32 FromIndex, int32 ToIndex)
{
	if (!InventoryComponent)
		return;

	InventoryComponent->SwapItems(FromIndex, ToIndex);
}

void UInventoryWidget::HandleEquipmentSlotInteraction(USlotWidget* SlotWidget, bool bIsRightClick)
{
	if (!InventoryComponent)
		return;
	
	// 장비 슬롯일 경우 주로 우클릭으로 장비 해제
	if (bIsRightClick && SlotWidget->ItemID != 0)
	{
		InventoryComponent->UnEquipItem(SlotWidget->EquipmentSlotType);
	}
}

void UInventoryWidget::HandleInventorySlotInteraction(USlotWidget* SlotWidget, bool bIsRightClick)
{
	if (!InventoryComponent)
		return;
	
	// 인벤토리 슬롯일 경우
	if (SlotWidget->ItemID != 0)
	{
		if (bIsRightClick)
		{
			// 우클릭으로 아이템 장착 시도
			const FItemData* ItemData = m_GameInstance->GetDataItem(SlotWidget->ItemID);
			if (ItemData && (ItemData->ItemCategory == EItemCategory::Equipment || 
							 ItemData->ItemCategory == EItemCategory::Weapon))
			{
				InventoryComponent->EquipItemByIndex(SlotWidget->SlotIndex);
			}
			// 소비 아이템인 경우 사용 (추가 구현 필요)
			// else if (ItemData && ItemData->ItemCategory == EItemCategory::Consumable)
			// {
			//     // 아이템 사용 구현
			// }
		}
		else
		{
			// 좌클릭 - 기본 선택 또는 특수 기능
			// 예: 아이템 분할, 상세 정보 보기 등 (추가 구현 필요)
		}
	}
}
