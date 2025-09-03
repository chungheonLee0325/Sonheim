// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"

#include "ConfirmWidget.h"
#include "SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"
#include "Sonheim/UI/Widget/GameObject/ContainerWidget.h"

#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"

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

	// Trash/Discard 존 바인딩
	if (DropZone)
	{
		DropZone->BindOwner(this);
		DropZone->SetDiscardMode(false);
	}
	if (DiscardZone)
	{
		DiscardZone->BindOwner(this);
		DiscardZone->SetDiscardMode(true);
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
		// 타겟 장비 슬롯으로 지정 장착(스왑 포함)
		InventoryComponent->EquipItemToSlotByIndex(FromSlot->SlotIndex, ToSlot->EquipmentSlotType);
	}
	// 장비 슬롯에서 인벤토리로 드래그 드롭
	else if (FromSlot->EquipmentSlotType != EEquipmentSlotType::None &&
		ToSlot->EquipmentSlotType == EEquipmentSlotType::None)
	{
		// 대상 인벤토리 칸이 비어있지 않다면 스왑 처리: 해당 인벤 아이템을 장비 슬롯으로 장착
		if (ToSlot->ItemID != 0)
		{
			InventoryComponent->EquipItemToSlotByIndex(ToSlot->SlotIndex, FromSlot->EquipmentSlotType);
		}
		else
		{
			// 비었으면 단순 해제
			InventoryComponent->UnEquipItem(FromSlot->EquipmentSlotType);
		}
	}
	// 장비 슬롯 간의 드래그 드롭 (무시하거나 필요한 경우 특별 처리)
	else if (FromSlot->EquipmentSlotType != EEquipmentSlotType::None &&
		ToSlot->EquipmentSlotType != EEquipmentSlotType::None)
	{
		// 같은 계열(무기끼리/악세끼리)만 스왑 허용
		auto IsWeaponSlot = [](EEquipmentSlotType T)
		{
			return T == EEquipmentSlotType::Weapon1 || T == EEquipmentSlotType::Weapon2 ||
				T == EEquipmentSlotType::Weapon3 || T == EEquipmentSlotType::Weapon4;
		};
		auto IsAccessorySlot = [](EEquipmentSlotType T)
		{
			return T == EEquipmentSlotType::Accessory1 || T == EEquipmentSlotType::Accessory2;
		};

		const bool bBothWeapon = IsWeaponSlot(FromSlot->EquipmentSlotType) && IsWeaponSlot(ToSlot->EquipmentSlotType);
		const bool bBothAccessory = IsAccessorySlot(FromSlot->EquipmentSlotType) && IsAccessorySlot(
			ToSlot->EquipmentSlotType);
		if (bBothWeapon || bBothAccessory)
		{
			InventoryComponent->SwapEquippedItems(FromSlot->EquipmentSlotType, ToSlot->EquipmentSlotType);
		}
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
	if (!InventoryComponent || !SlotWidget)
		return;

	// 인벤토리 슬롯일 경우
	if (SlotWidget->ItemID != 0)
	{
		if (bIsRightClick)
		{
			// 상자 모드 체크
			if (bIsContainerMode && CurrentOpenContainer)
			{
				// 상자로 아이템 전송
				if (m_PlayerController)
				{
					m_PlayerController->Server_PlayerContainerTransfer(
						CurrentOpenContainer,
						false, // Player to Container
						SlotWidget->ItemID,
						SlotWidget->Quantity,
						SlotWidget->SlotIndex
					);
				}
				// 상자로 보냈으므로 더 이상 처리하지 않음
				return;
			}

			// 우클릭으로 아이템 장착 시도
			const FItemData* ItemData = m_GameInstance->GetDataItem(SlotWidget->ItemID);
			if (ItemData && (ItemData->ItemCategory == EItemCategory::Equipment ||
				ItemData->ItemCategory == EItemCategory::Weapon))
			{
				InventoryComponent->EquipItemByIndex(SlotWidget->SlotIndex);
			}
			// ToDo : 임시코드 - 베타 발표용
			else if (ItemData->ItemID == 1 || ItemData->ItemID == 5 || ItemData->ItemID == 10 || ItemData->ItemID == 15)
			{
				int itemCount = InventoryComponent->GetItemCount(ItemData->ItemID);
				InventoryComponent->RemoveItem(ItemData->ItemID, itemCount);
				InventoryComponent->GetSonheimPlayer()->RestoreStair(ItemData->ItemID, itemCount);
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

void UInventoryWidget::HandleExternalDrop(USlotWidget* FromSlot, USlotWidget* ToSlot)
{
	if (!FromSlot || !InventoryComponent || !ToSlot)
		return;

	// 상자에서 가져온 아이템인지 확인
	UContainerWidget* FromContainer = Cast<UContainerWidget>(FromSlot->GetTypedOuter<UContainerWidget>());
	if (FromContainer && FromContainer->GetContainerComponent())
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetOwningPlayer()))
		{
			// 장비 슬롯으로 드롭
			if (ToSlot->EquipmentSlotType != EEquipmentSlotType::None)
			{
				if (ToSlot->ItemID != 0)
				{
					// 상자칸 ↔ 장비칸 스왑 (장비 슬롯은 음수 인코딩)
					const int32 EncodedEquip = -(1 + static_cast<int32>(ToSlot->EquipmentSlotType));
					PC->Server_ContainerOperation(FromContainer->GetOwningContainer(),
					                              EContainerOperation::SwapWithPlayer,
					                              FromSlot->SlotIndex, EncodedEquip);
				}
				else
				{
					// 상자 → 장비 슬롯 직접 장착
					PC->Server_ContainerOperation(FromContainer->GetOwningContainer(),
					                              EContainerOperation::TransferToPlayer,
					                              FromSlot->SlotIndex, static_cast<int32>(ToSlot->EquipmentSlotType));
				}
			}
			else
			{
				// 인벤토리 슬롯으로 드롭
				if (ToSlot->ItemID != 0)
				{
					// 상자칸 ↔ 인벤칸 스왑
					PC->Server_ContainerOperation(FromContainer->GetOwningContainer(),
					                              EContainerOperation::SwapWithPlayer,
					                              FromSlot->SlotIndex, ToSlot->SlotIndex);
				}
				else
				{
					// 상자 → 인벤토리 이동
					PC->Server_ContainerOperation(FromContainer->GetOwningContainer(),
					                              EContainerOperation::TransferToPlayer,
					                              FromSlot->SlotIndex, -1);
				}
			}
		}
	}
}

void UInventoryWidget::HandleTrashDrop(USlotWidget* FromSlot, bool bDiscardMode)
{
	if (!FromSlot || !InventoryComponent) return;

	// 팝업 생성
	if (!ConfirmClass) return;
	UConfirmWidget* W = CreateWidget<UConfirmWidget>(this, ConfirmClass);
	if (!W) return;

	PendingFromSlot = FromSlot;
	PendingDiscardMode = bDiscardMode;

	// 수량 선택 필요(스택형)는 팝업에서 판단하도록 MaxCount=슬롯 수량 전달
	W->Setup(FromSlot->ItemID, FromSlot->Quantity, bDiscardMode);
	W->OnConfirm.AddDynamic(this, &UInventoryWidget::OnConfirmTrashAction);
	W->AddToViewport();
}

void UInventoryWidget::SetContainerMode(bool bEnabled, class ABaseContainer* Container, ASonheimPlayerController* PC)
{
	bIsContainerMode = bEnabled;
	CurrentOpenContainer = Container;
	m_PlayerController = PC;
}

void UInventoryWidget::OnConfirmTrashAction(int32 Count, bool bDiscardMode)
{
	if (!InventoryComponent || !PendingFromSlot.IsValid()) return;

	const int32 Index = PendingFromSlot.Get()->SlotIndex;
	const int32 UseCount = FMath::Max(1, Count);

	if (bDiscardMode)
	{
		// 폐기(삭제)
		InventoryComponent->ServerDiscardItemByIndex(Index, UseCount);
	}
	else
	{
		// 버리기(바닥 드롭)
		InventoryComponent->ServerDropItemByIndex(Index, UseCount);
	}

	PendingFromSlot.Reset();
}
