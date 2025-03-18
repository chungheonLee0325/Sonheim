// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"


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
	m_Player = Cast<ASonheimPlayer>(GetOwner());
	
	// 장비 슬롯 초기화
	for (EEquipmentSlotType Value : TEnumRange<EEquipmentSlotType>())
	{
		EEquipmentSlotType SlotType = Value;
		if (SlotType != EEquipmentSlotType::None)
		{
			EquippedItems.Add(SlotType, 0); // 0은 비어있음을 의미
		}
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
	
	if (ExistingItemIndex != INDEX_NONE)
	{
		// 이미 있는 아이템, 수량 증가
		InventoryItems[ExistingItemIndex].Count += ItemCount;
	}
	else
	{
		// 새 아이템 추가
		if (InventoryItems.Num() >= MaxInventorySlots)
			return false; // 인벤토리 가득 참
			
		InventoryItems.Add(FInventoryItem(ItemID, ItemCount));
	}
	
	// 이벤트 발생
	OnItemAdded.Broadcast(ItemID, ItemCount);
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
		// 장착된 아이템이면 장착 해제
		if (Item.bIsEquipped)
		{
			FItemData* ItemData = GetItemData(ItemID);
			if (ItemData && ItemData->EquipSlot != EEquipmentSlotType::None)
			{
				UnequipItem(ItemData->EquipSlot);
			}
		}
		
		InventoryItems.RemoveAt(ItemIndex);
	}
	
	// 이벤트 발생
	OnItemRemoved.Broadcast(ItemID, ItemCount);
	BroadcastInventoryChanged();
	
	return true;
}

bool UInventoryComponent::EquipItem(int ItemID)
{
	if (!IsValidItemID(ItemID))
		return false;
		
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData || ItemData->EquipSlot == EEquipmentSlotType::None)
		return false; // 장착 불가능한 아이템
		
	// 인벤토리에서 아이템 찾기
	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false; // 아이템 없음
		
	// 같은 슬롯에 장착된 아이템이 있으면 해제
	if (EquippedItems.Contains(ItemData->EquipSlot) && EquippedItems[ItemData->EquipSlot] != 0)
	{
		UnequipItem(ItemData->EquipSlot);
	}
	
	// 아이템 장착
	EquippedItems[ItemData->EquipSlot] = ItemID;
	InventoryItems[ItemIndex].bIsEquipped = true;
	
	// 스탯 적용
	ApplyEquipmentStats(ItemID, true);
	
	// 이벤트 발생
	OnEquipmentChanged.Broadcast(ItemData->EquipSlot, ItemID);
	BroadcastInventoryChanged();
	
	return true;
}

bool UInventoryComponent::UnequipItem(EEquipmentSlotType SlotType)
{
	if (SlotType == EEquipmentSlotType::None || !EquippedItems.Contains(SlotType))
		return false;
		
	int ItemID = EquippedItems[SlotType];
	if (ItemID == 0)
		return false; // 슬롯이 비어있음
		
	// 인벤토리에서 아이템 찾기
	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex != INDEX_NONE)
	{
		InventoryItems[ItemIndex].bIsEquipped = false;
	}
	
	// 스탯 제거
	ApplyEquipmentStats(ItemID, false);
	
	// 장착 해제
	EquippedItems[SlotType] = 0;
	
	// 이벤트 발생
	OnEquipmentChanged.Broadcast(SlotType, 0);
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

TMap<EEquipmentSlotType, int> UInventoryComponent::GetEquippedItems() const
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
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData || !m_Player)
		return;
		
	float Multiplier = bEquipping ? 1.0f : -1.0f;
	
	// 스탯 적용 (실제 구현은 플레이어 클래스에 맞게 조정 필요)
	if (ItemData->HPBonus != 0)
	{
		// 최대 체력 조정
		// m_Player->UpdateMaxHP(ItemData->HPBonus * Multiplier);
	}
	
	if (ItemData->StaminaBonus != 0)
	{
		// 최대 스태미나 조정
		// m_Player->UpdateMaxStamina(ItemData->StaminaBonus * Multiplier);
	}
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

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast(InventoryItems);
}

bool UInventoryComponent::EquipItemByIndex(int32 InventoryIndex)
{
	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num())
		return false;
		
	FInventoryItem& Item = InventoryItems[InventoryIndex];
	FItemData* ItemData = GetItemData(Item.ItemID);
	
	if (!ItemData || ItemData->EquipSlot == EEquipmentSlotType::None)
		return false; // 장착 불가능한 아이템
		
	// 같은 슬롯에 장착된 아이템 인덱스 찾기
	int32 PreviousItemIndex = GetEquippedItemIndex(ItemData->EquipSlot);
	if (PreviousItemIndex != INDEX_NONE)
	{
		// 이미 장착된 아이템 해제
		InventoryItems[PreviousItemIndex].bIsEquipped = false;
	}
	
	// 아이템 장착
	EquippedItems[ItemData->EquipSlot] = Item.ItemID;
	Item.bIsEquipped = true;
	
	// 스탯 적용
	ApplyEquipmentStats(Item.ItemID, true);
	
	// 이벤트 발생
	OnEquipmentChanged.Broadcast(ItemData->EquipSlot, Item.ItemID);
	BroadcastInventoryChanged();
	
	return true;
}

int32 UInventoryComponent::GetEquippedItemIndex(EEquipmentSlotType SlotType) const
{
	if (SlotType == EEquipmentSlotType::None || !EquippedItems.Contains(SlotType))
		return INDEX_NONE;
		
	int32 ItemID = EquippedItems[SlotType];
	if (ItemID == 0)
		return INDEX_NONE;
		
	// 인벤토리에서 해당 ID를 가진 장착된 아이템 찾기
	for (int32 i = 0; i < InventoryItems.Num(); i++)
	{
		if (InventoryItems[i].ItemID == ItemID && InventoryItems[i].bIsEquipped)
			return i;
	}
	
	return INDEX_NONE;
}

bool UInventoryComponent::EquipWeaponToSlot(int32 InventoryIndex, EEquipmentSlotType WeaponSlot)
{
	// 유효한 무기 슬롯인지 확인
	if (WeaponSlot != EEquipmentSlotType::Weapon1 && 
		WeaponSlot != EEquipmentSlotType::Weapon2 && 
		WeaponSlot != EEquipmentSlotType::Weapon3 && 
		WeaponSlot != EEquipmentSlotType::Weapon4)
		return false;
		
	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num())
		return false;
		
	FInventoryItem& Item = InventoryItems[InventoryIndex];
	FItemData* ItemData = GetItemData(Item.ItemID);
	
	// 무기 아이템인지 확인
	if (!ItemData || ItemData->ItemCategory != EItemCategory::Weapon)
		return false;
		
	// 이전에 같은 슬롯에 장착된 무기 찾아서 해제
	int32 PreviousItemIndex = GetEquippedItemIndex(WeaponSlot);
	if (PreviousItemIndex != INDEX_NONE)
	{
		InventoryItems[PreviousItemIndex].bIsEquipped = false;
		ApplyEquipmentStats(InventoryItems[PreviousItemIndex].ItemID, false);
	}
	
	// 새 무기 장착
	EquippedItems[WeaponSlot] = Item.ItemID;
	Item.bIsEquipped = true;
	
	// 스탯 적용
	ApplyEquipmentStats(Item.ItemID, true);
	
	// 이벤트 발생
	OnEquipmentChanged.Broadcast(WeaponSlot, Item.ItemID);
	BroadcastInventoryChanged();
	
	return true;
}

void UInventoryComponent::SwitchWeaponSlot(EEquipmentSlotType NewWeaponSlot)
{
	// 유효한 무기 슬롯인지 확인
	if (NewWeaponSlot != EEquipmentSlotType::Weapon1 && 
		NewWeaponSlot != EEquipmentSlotType::Weapon2 && 
		NewWeaponSlot != EEquipmentSlotType::Weapon3 && 
		NewWeaponSlot != EEquipmentSlotType::Weapon4)
		return;
		
	CurrentWeaponSlot = NewWeaponSlot;
	
	// 무기 변경 이벤트 호출 가능
	OnEquipmentChanged.Broadcast(CurrentWeaponSlot, EquippedItems.Contains(CurrentWeaponSlot) ? EquippedItems[CurrentWeaponSlot] : 0);
}

