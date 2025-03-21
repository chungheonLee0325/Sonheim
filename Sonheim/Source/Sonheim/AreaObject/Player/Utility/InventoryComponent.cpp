// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "Sonheim/AreaObject/Attribute/StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 인스턴스와 플레이어 참조 얻기
	m_GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
	m_PlayerState = Cast<ASonheimPlayerState>(GetOwner());

	// 장비 슬롯 초기화
	for (EEquipmentSlotType Value : TEnumRange<EEquipmentSlotType>())
	{
		EEquipmentSlotType SlotType = Value;
		FInventoryItem InventoryItem{};
		if (SlotType != EEquipmentSlotType::None)
		{
			EquippedItems.Add(SlotType, InventoryItem); // 0은 비어있음을 의미
		}
	}
	
	// StatBonusComponent에 초기 무기 슬롯 설정
	if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
	{
		m_PlayerState->m_StatBonusComponent->SetCurrentWeaponSlot(CurrentWeaponSlot);
	}
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UInventoryComponent::AddItem(int ItemID, int ItemCount)
{
	if (ItemCount <= 0 || !IsValidItemID(ItemID))
		return false;

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
		return false;

	// 인벤토리에서 같은 아이템 찾기
	int ExistingItemIndex = FindItemIndexInInventory(ItemID);

	if (ExistingItemIndex != INDEX_NONE && ItemData->bStackable == true)
	{
		// 이미 있는 아이템, 수량 증가
		InventoryItems[ExistingItemIndex].Count += ItemCount;
		// 이벤트 발생
		OnItemAdded.Broadcast(ItemID, InventoryItems[ExistingItemIndex].Count);
	}
	else
	{
		// 새 아이템 추가
		if (InventoryItems.Num() >= MaxInventorySlots)
			return false; // 인벤토리 가득 참

		InventoryItems.Add(FInventoryItem(ItemID, ItemCount));
		// 이벤트 발생
		OnItemAdded.Broadcast(ItemID, ItemCount);
	}

	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::AddItemByInventoryItem(FInventoryItem InventoryItem)
{
	FItemData* ItemData = GetItemData(InventoryItem.ItemID);
	if (!ItemData)
		return false;

	// 인벤토리에서 같은 아이템 찾기
	int ExistingItemIndex = FindItemIndexInInventory(InventoryItem.ItemID);

	if (ExistingItemIndex != INDEX_NONE && ItemData->bStackable == true)
	{
		// 이미 있는 아이템, 수량 증가
		InventoryItems[ExistingItemIndex].Count += InventoryItem.Count;
		// 이벤트 발생
		OnItemAdded.Broadcast(InventoryItem.ItemID, InventoryItems[ExistingItemIndex].Count);
	}
	else
	{
		// 새 아이템 추가
		if (InventoryItems.Num() >= MaxInventorySlots)
			return false; // 인벤토리 가득 참

		InventoryItems.Add(InventoryItem);
		// 이벤트 발생
		OnItemAdded.Broadcast(InventoryItem.ItemID, InventoryItem.Count);
	}

	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::RemoveItem(int ItemID, int ItemCount)
{
	if (ItemCount <= 0 || !IsValidItemID(ItemID))
		return false;

	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false; // 아이템 없음

	FInventoryItem& Item = InventoryItems[ItemIndex];

	if (Item.Count < ItemCount)
		return false; // 충분한 수량 없음

	// 수량 감소
	Item.Count -= ItemCount;

	// 수량이 0이면 제거
	if (Item.Count <= 0)
	{
		// 장착된 아이템이면 장착 해제:: ToDo: 프로젝트마다 customize 될 부분
		//if (Item.bIsEquipped)
		//{
		//	FItemData* ItemData = GetItemData(ItemID);
		//	EEquipmentSlotType EquipSlotType = FindEmptySlotForType(ItemData->EquipmentData.EquipKind);
		//	if (ItemData && EquipSlotType != EEquipmentSlotType::None)
		//	{
		//		UnEquipItem(EquipSlotType);
		//	}
		//}

		InventoryItems.RemoveAt(ItemIndex);
	}

	// 이벤트 발생
	OnItemRemoved.Broadcast(ItemID, Item.Count);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::RemoveItemByIndex(int Index)
{
	if (InventoryItems.IsEmpty())
		return false;

	int ItemID = InventoryItems[Index].ItemID;
	int Count = InventoryItems[Index].Count;

	InventoryItems.RemoveAt(Index);
	// 이벤트 발생
	OnItemRemoved.Broadcast(ItemID, Count);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::EquipItem(int ItemID)
{
	if (!IsValidItemID(ItemID))
		return false;

	FItemData* ItemData = GetItemData(ItemID);
	EEquipmentSlotType EquipSlotType = FindEmptySlotForType(ItemData->EquipmentData.EquipKind);
	if (!ItemData || EquipSlotType == EEquipmentSlotType::None)
		return false; // 장착 불가능한 아이템

	// 인벤토리에서 아이템 찾기
	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false; // 아이템 없음

	// 같은 슬롯에 장착된 아이템이 있으면 해제
	if (EquippedItems.Contains(EquipSlotType) && EquippedItems[EquipSlotType].ItemID != 0)
	{
		UnEquipItem(EquipSlotType);
	}

	// 아이템 장착
	InventoryItems[ItemIndex].bIsEquipped = true;
	FInventoryItem Item = InventoryItems[ItemIndex];
	EquippedItems[EquipSlotType] = Item;

	// 스탯 적용 - 무기와 일반 장비를 구분하여 처리
	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(EquipSlotType, ItemID, true);
	}
	else
	{
		ApplyEquipmentStats(ItemID, true);
	}

	// Inventory에서 아이템 삭제 :: ToDo: 프로젝트마다 customize 될 부분
	RemoveItemByIndex(ItemIndex);

	// 이벤트 발생
	OnEquipmentChanged.Broadcast(EquipSlotType, Item);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::UnEquipItem(EEquipmentSlotType SlotType)
{
	if (SlotType == EEquipmentSlotType::None || !EquippedItems.Contains(SlotType))
		return false;

	int ItemID = EquippedItems[SlotType].ItemID;
	if (ItemID == 0)
		return false;

	// 인벤토리에 아이템 추가:: ToDo: 프로젝트마다 customize 될 부분
	EquippedItems[SlotType].bIsEquipped = false;
	AddItemByInventoryItem(EquippedItems[SlotType]);

	// 인벤토리에서 아이템 찾기 :: ToDo: 프로젝트마다 customize 될 부분
	// int ItemIndex = FindItemIndexInInventory(ItemID);
	// if (ItemIndex != INDEX_NONE)
	// {
	// 	InventoryItems[ItemIndex].bIsEquipped = false;
	// }

	// 스탯 제거 - 무기와 일반 장비를 구분하여 처리
	FItemData* ItemData = GetItemData(ItemID);
	if (ItemData && ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(SlotType, ItemID, false);
	}
	else
	{
		ApplyEquipmentStats(ItemID, false);
	}

	// 장착 해제
	EquippedItems[SlotType] = FInventoryItem();

	// 이벤트 발생
	OnEquipmentChanged.Broadcast(SlotType, EquippedItems[SlotType]);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::HasItem(int ItemID, int RequiredCount) const
{
	if (RequiredCount <= 0 || !IsValidItemID(ItemID))
		return false;

	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false;

	return InventoryItems[ItemIndex].Count >= RequiredCount;
}

int UInventoryComponent::GetItemCount(int ItemID) const
{
	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return 0;

	return InventoryItems[ItemIndex].Count;
}

TArray<FInventoryItem> UInventoryComponent::GetInventory() const
{
	return InventoryItems;
}

TMap<EEquipmentSlotType, FInventoryItem> UInventoryComponent::GetEquippedItems() const
{
	return EquippedItems;
}

bool UInventoryComponent::IsValidItemID(int ItemID) const
{
	return m_GameInstance && m_GameInstance->GetDataItem(ItemID) != nullptr;
}

FItemData* UInventoryComponent::GetItemData(int ItemID) const
{
	return m_GameInstance ? m_GameInstance->GetDataItem(ItemID) : nullptr;
}

void UInventoryComponent::ApplyEquipmentStats(int ItemID, bool bEquipping)
{
	m_PlayerState->m_StatBonusComponent->ApplyItemStatBonuses(ItemID, bEquipping);
}

int UInventoryComponent::FindItemIndexInInventory(int ItemID) const
{
	for (int i = 0; i < InventoryItems.Num(); i++)
	{
		if (InventoryItems[i].ItemID == ItemID)
			return i;
	}

	return INDEX_NONE;
}

EEquipmentSlotType UInventoryComponent::FindEmptySlotForType(EEquipmentKindType ItemType)
{
	switch (ItemType)
	{
	case EEquipmentKindType::Head:
		return EEquipmentSlotType::Head;
	//return EquippedItems[EEquipmentSlotType::Head].IsEmpty() ?  EEquipmentSlotType::Head: EEquipmentSlotType::None;

	case EEquipmentKindType::Body:
		return EEquipmentSlotType::Body;
	//return EquippedItems[EEquipmentSlotType::Body].IsEmpty() ? EEquipmentSlotType::Body : EEquipmentSlotType::None;

	case EEquipmentKindType::Weapon:
		// 무기 슬롯 4개 중 빈 슬롯 찾기
		if (EquippedItems[EEquipmentSlotType::Weapon1].IsEmpty())
			return EEquipmentSlotType::Weapon1;
		if (EquippedItems[EEquipmentSlotType::Weapon2].IsEmpty())
			return EEquipmentSlotType::Weapon2;
		if (EquippedItems[EEquipmentSlotType::Weapon3].IsEmpty())
			return EEquipmentSlotType::Weapon3;
		if (EquippedItems[EEquipmentSlotType::Weapon4].IsEmpty())
			return EEquipmentSlotType::Weapon4;
		return EEquipmentSlotType::Weapon1;

	case EEquipmentKindType::Accessory:
		// 악세서리 슬롯 2개 중 빈 슬롯 찾기
		if (EquippedItems[EEquipmentSlotType::Accessory1].IsEmpty())
			return EEquipmentSlotType::Accessory1;
		if (EquippedItems[EEquipmentSlotType::Accessory2].IsEmpty())
			return EEquipmentSlotType::Accessory2;
		return EEquipmentSlotType::None;

	case EEquipmentKindType::Glider:
		return EEquipmentSlotType::Glider;
	//return EquippedItems[EEquipmentSlotType::Glider].IsEmpty()? EEquipmentSlotType::Glider: EEquipmentSlotType::None;

	case EEquipmentKindType::Shield:
		return EEquipmentSlotType::Shield;
	//return EquippedItems[EEquipmentSlotType::Shield].IsEmpty() ? EEquipmentSlotType::Shield  : EEquipmentSlotType::None;

	case EEquipmentKindType::SphereModule:
		return EEquipmentSlotType::SphereModule;
	//return EquippedItems[EEquipmentSlotType::SphereModule].IsEmpty()? EEquipmentSlotType::SphereModule: EEquipmentSlotType::None;

	default:
		return EEquipmentSlotType::None;
	}
}

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast(InventoryItems);
}

bool UInventoryComponent::EquipItemByIndex(int32 InventoryIndex)
{
	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num())
		return false;

	FInventoryItem Item = InventoryItems[InventoryIndex];
	FItemData* ItemData = GetItemData(Item.ItemID);

	EEquipmentSlotType EquipSlotType = FindEmptySlotForType(ItemData->EquipmentData.EquipKind);
	if (!ItemData || EquipSlotType == EEquipmentSlotType::None)
		return false; // 장착 불가능한 아이템

	// 같은 슬롯에 장착된 아이템이 있으면 해제
	if (!EquippedItems[EquipSlotType].IsEmpty())
	{
		UnEquipItem(EquipSlotType);
	}

	// 아이템 장착
	Item.bIsEquipped = true;
	EquippedItems[EquipSlotType] = Item;


	// 스탯 적용 - 무기와 일반 장비를 구분하여 처리
	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(EquipSlotType, Item.ItemID, true);
	}
	else
	{
		ApplyEquipmentStats(Item.ItemID, true);
	}


	// Inventory에서 아이템 삭제 :: ToDo: 프로젝트마다 customize 될 부분
	RemoveItemByIndex(InventoryIndex);

	// 이벤트 발생
	OnEquipmentChanged.Broadcast(EquipSlotType, Item);
	BroadcastInventoryChanged();

	return true;
}

FInventoryItem UInventoryComponent::GetEquippedItem(EEquipmentSlotType SlotType) const
{
	return EquippedItems[SlotType];
	// if (SlotType == EEquipmentSlotType::None || !EquippedItems.Contains(SlotType))
	// 	return INDEX_NONE;
	//
	// int32 ItemID = EquippedItems[SlotType];
	// if (ItemID == 0)
	// 	return INDEX_NONE;
	//
	// // 인벤토리에서 해당 ID를 가진 장착된 아이템 찾기
	// for (int32 i = 0; i < InventoryItems.Num(); i++)
	// {
	// 	if (InventoryItems[i].ItemID == ItemID && InventoryItems[i].bIsEquipped)
	// 		return i;
	// }
	//
	// return INDEX_NONE;
}


void UInventoryComponent::SwitchWeaponSlot(int Index)
{
	int minIndex = static_cast<int>(EEquipmentSlotType::Weapon1);
	int maxIndex = static_cast<int>(EEquipmentSlotType::Weapon4);
	int currentIndex = static_cast<int>(CurrentWeaponSlot);

	// Weapon1 ~ Weapon4 의 범위를 0 ~ 3으로 변환
	int normalizedIndex = currentIndex - minIndex;

	// Index 를 추가하고, 0 ~ 3 범위에서 순환
	normalizedIndex = (normalizedIndex + Index) % 4;
	if (normalizedIndex < 0) normalizedIndex += 4; // 음수일 때 보정

	// 다시 원래 Enum 범위로 변환 (Weapon1 ~ Weapon4)
	EEquipmentSlotType newWeaponSlot = static_cast<EEquipmentSlotType>(normalizedIndex + minIndex);
	
	// 무기 슬롯이 실제로 변경된 경우에만 처리
	if (CurrentWeaponSlot != newWeaponSlot)
	{
		CurrentWeaponSlot = newWeaponSlot;
		
		// StatBonusComponent에 현재 무기 슬롯 업데이트
		m_PlayerState->m_StatBonusComponent->SetCurrentWeaponSlot(CurrentWeaponSlot);
		
		// 무기 변경 이벤트 호출
		OnWeaponChanged.Broadcast(CurrentWeaponSlot, GetEquippedItem(CurrentWeaponSlot).ItemID);
	}
}

FItemData* UInventoryComponent::GetCurrentWeaponData()
{
	auto item = GetEquippedItem(CurrentWeaponSlot);

	return m_GameInstance->GetDataItem(item.ItemID);
}

bool UInventoryComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
	// 인덱스 유효성 검사
	if (FromIndex < 0 || FromIndex >= InventoryItems.Num() ||
		ToIndex < 0 || ToIndex >= InventoryItems.Num())
		return false;

	// 같은 인덱스인 경우 무시
	if (FromIndex == ToIndex)
		return true;

	// 아이템 스왑
	FInventoryItem TempItem = InventoryItems[FromIndex];
	InventoryItems[FromIndex] = InventoryItems[ToIndex];
	InventoryItems[ToIndex] = TempItem;

	// 이벤트 발생
	BroadcastInventoryChanged();

	return true;
}
