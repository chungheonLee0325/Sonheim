// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComponent.h"
#include "Sonheim/AreaObject/Attribute/StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 조건부 복제로 네트워크 트래픽 감소
	DOREPLIFETIME_CONDITION(UInventoryComponent, InventoryItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComponent, EquippedSlots, COND_OwnerOnly);
	DOREPLIFETIME(UInventoryComponent, CurrentWeaponSlot); // 모든 클라이언트가 볼 수 있어야 함
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 인스턴스와 플레이어 참조 얻기
	m_GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
	m_PlayerState = Cast<ASonheimPlayerState>(GetOwner());

	// 서버에서만 초기화
	if (GetOwnerRole() == ROLE_Authority)
	{
		// 장비 슬롯 초기화
		for (uint8 i = 0; i < (uint8)EEquipmentSlotType::Max; i++)
		{
			EEquipmentSlotType SlotType = (EEquipmentSlotType)i;
			if (SlotType != EEquipmentSlotType::None)
			{
				EquippedSlots.Add(FEquippedSlot(SlotType, FInventoryItem()));
			}
		}

		// StatBonusComponent에 초기 무기 슬롯 설정
		if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
		{
			m_PlayerState->m_StatBonusComponent->SetCurrentWeaponSlot(CurrentWeaponSlot);
		}
	}
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 참조 정리
	m_Player = nullptr;
	m_PlayerState = nullptr;
	m_GameInstance = nullptr;
	
	// 델리게이트 정리
	OnInventoryChanged.Clear();
	OnEquipmentChanged.Clear();
	OnWeaponChanged.Clear();
	OnItemAdded.Clear();
	OnItemRemoved.Clear();
	
	Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// OnRep 함수들
void UInventoryComponent::OnRep_InventoryItems()
{
	BroadcastInventoryChanged();
	
	#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
			FString::Printf(TEXT("[Client] Inventory Updated: %d items"), InventoryItems.Num()));
	}
	#endif
}

void UInventoryComponent::OnRep_EquippedItems()
{
	// 장착 아이템 변경 이벤트 호출
	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		OnEquipmentChanged.Broadcast(Slot.SlotType, Slot.Item);
	}
}

void UInventoryComponent::OnRep_CurrentWeaponSlot()
{
	FInventoryItem CurrentWeapon = GetEquippedItem(CurrentWeaponSlot);
	OnWeaponChanged.Broadcast(CurrentWeaponSlot, CurrentWeapon.ItemID);
}

// 서버 RPC 구현
void UInventoryComponent::ServerAddItem_Implementation(int ItemID, int ItemCount)
{
	AddItem(ItemID, ItemCount);
}

void UInventoryComponent::ServerRemoveItem_Implementation(int ItemID, int ItemCount)
{
	RemoveItem(ItemID, ItemCount);
}

void UInventoryComponent::ServerRemoveItemByIndex_Implementation(int Index)
{
	RemoveItemByIndex(Index);
}

void UInventoryComponent::ServerEquipItem_Implementation(int ItemID)
{
	EquipItem(ItemID);
}

void UInventoryComponent::ServerEquipItemByIndex_Implementation(int32 InventoryIndex)
{
	EquipItemByIndex(InventoryIndex);
}

void UInventoryComponent::ServerUnEquipItem_Implementation(EEquipmentSlotType SlotType)
{
	UnEquipItem(SlotType);
}

void UInventoryComponent::ServerSwitchWeaponSlot_Implementation(int Index)
{
	SwitchWeaponSlot(Index);
}

void UInventoryComponent::ServerSwapItems_Implementation(int32 FromIndex, int32 ToIndex)
{
	SwapItems(FromIndex, ToIndex);
}

// 데이터 유효성 검증 함수들
bool UInventoryComponent::ValidateItemCount(int ItemCount) const
{
	return ItemCount > 0 && ItemCount <= MaxItemStackCount;
}

bool UInventoryComponent::ValidateItemOperation(int ItemID, int ItemCount) const
{
	if (!IsValidItemID(ItemID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid ItemID: %d"), ItemID);
		return false;
	}
	
	if (!ValidateItemCount(ItemCount))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid item count: %d (Max: %d)"), ItemCount, MaxItemStackCount);
		return false;
	}
	
	return true;
}

// 인벤토리 함수들
bool UInventoryComponent::AddItem(int ItemID, int ItemCount)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerAddItem(ItemID, ItemCount);
		return false;
	}

	// 데이터 유효성 검증
	if (!ValidateItemOperation(ItemID, ItemCount))
		return false;

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
		return false;

	// 인벤토리에서 같은 아이템 찾기
	int ExistingItemIndex = FindItemIndexInInventory(ItemID);

	if (ExistingItemIndex != INDEX_NONE && ItemData->bStackable)
	{
		// 스택 제한 확인
		int NewCount = InventoryItems[ExistingItemIndex].Count + ItemCount;
		if (NewCount > MaxItemStackCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Stack overflow prevented: %d + %d > %d"), 
				InventoryItems[ExistingItemIndex].Count, ItemCount, MaxItemStackCount);
			return false;
		}
		
		// 이미 있는 아이템, 수량 증가
		InventoryItems[ExistingItemIndex].Count = NewCount;
		OnItemAdded.Broadcast(ItemID, InventoryItems[ExistingItemIndex].Count);
	}
	else
	{
		// 새 아이템 추가
		if (InventoryItems.Num() >= MaxInventorySlots)
		{
			UE_LOG(LogTemp, Warning, TEXT("Inventory full: %d/%d"), InventoryItems.Num(), MaxInventorySlots);
			return false;
		}

		FInventoryItem NewItem(ItemID, ItemCount);
		InventoryItems.Add(NewItem);
		OnItemAdded.Broadcast(ItemID, ItemCount);
	}

	#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		FString RoleString = GetOwnerRole() == ROLE_Authority ? TEXT("Server") : TEXT("Client");
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
			FString::Printf(TEXT("[%s] Item Added: %d x%d"), *RoleString, ItemID, ItemCount));
	}
	#endif

	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::AddItemByInventoryItem(const FInventoryItem& InventoryItem)
{
	return AddItem(InventoryItem.ItemID, InventoryItem.Count);
}

bool UInventoryComponent::RemoveItem(int ItemID, int ItemCount)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerRemoveItem(ItemID, ItemCount);
		return false;
	}

	// 데이터 유효성 검증
	if (!ValidateItemOperation(ItemID, ItemCount))
		return false;

	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false;

	FInventoryItem& Item = InventoryItems[ItemIndex];

	if (Item.Count < ItemCount)
		return false;

	// 수량 감소
	Item.Count -= ItemCount;

	// 수량이 0이면 제거
	if (Item.Count <= 0)
	{
		InventoryItems.RemoveAt(ItemIndex);
	}

	OnItemRemoved.Broadcast(ItemID, Item.Count);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::RemoveItemByIndex(int Index)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerRemoveItemByIndex(Index);
		return false;
	}

	if (Index < 0 || Index >= InventoryItems.Num())
		return false;

	int ItemID = InventoryItems[Index].ItemID;
	int Count = InventoryItems[Index].Count;

	InventoryItems.RemoveAt(Index);
	
	OnItemRemoved.Broadcast(ItemID, Count);
	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::EquipItem(int ItemID)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerEquipItem(ItemID);
		return false;
	}

	if (!IsValidItemID(ItemID))
		return false;

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
		return false;

	EEquipmentSlotType EquipSlotType = FindEmptySlotForType(ItemData->EquipmentData.EquipKind);
	if (EquipSlotType == EEquipmentSlotType::None)
		return false;

	// 인벤토리에서 아이템 찾기
	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false;

	// 같은 슬롯에 장착된 아이템이 있으면 해제
	FInventoryItem* ExistingItem = GetEquippedSlotItem(EquipSlotType);
	if (ExistingItem && ExistingItem->ItemID != 0)
	{
		UnEquipItem(EquipSlotType);
	}

	// 아이템 장착
	FInventoryItem Item = InventoryItems[ItemIndex];
	Item.bIsEquipped = true;
	SetEquippedSlot(EquipSlotType, Item);

	// 스탯 적용
	if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
	{
		if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
		{
			m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(EquipSlotType, ItemID, true);
		}
		else
		{
			ApplyEquipmentStats(ItemID, true);
		}
	}

	// Inventory에서 아이템 삭제
	RemoveItemByIndex(ItemIndex);

	// 이벤트 발생
	OnEquipmentChanged.Broadcast(EquipSlotType, Item);
	
	// 무기인 경우 무기 변경 이벤트도 발생
	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		OnWeaponChanged.Broadcast(EquipSlotType, ItemID);
	}

	BroadcastInventoryChanged();

	return true;
}

bool UInventoryComponent::EquipItemByIndex(int32 InventoryIndex)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerEquipItemByIndex(InventoryIndex);
		return false;
	}

	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num())
		return false;

	int ItemID = InventoryItems[InventoryIndex].ItemID;
	return EquipItem(ItemID);
}

bool UInventoryComponent::UnEquipItem(EEquipmentSlotType SlotType)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerUnEquipItem(SlotType);
		return false;
	}

	if (SlotType == EEquipmentSlotType::None)
		return false;

	FInventoryItem* EquippedItem = GetEquippedSlotItem(SlotType);
	if (!EquippedItem || EquippedItem->ItemID == 0)
		return false;

	int ItemID = EquippedItem->ItemID;
	FInventoryItem ItemToReturn = *EquippedItem;
	ItemToReturn.bIsEquipped = false;

	// 인벤토리에 공간이 있는지 확인
	if (InventoryItems.Num() >= MaxInventorySlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot unequip: Inventory full"));
		return false;
	}

	// 스탯 제거
	if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
	{
		FItemData* ItemData = GetItemData(ItemID);
		if (ItemData && ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
		{
			m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(SlotType, ItemID, false);
		}
		else
		{
			ApplyEquipmentStats(ItemID, false);
		}
	}

	// 장착 해제
	RemoveEquippedSlot(SlotType);

	// 인벤토리에 아이템 추가
	AddItemByInventoryItem(ItemToReturn);

	// 이벤트 발생
	OnEquipmentChanged.Broadcast(SlotType, FInventoryItem());

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
	TMap<EEquipmentSlotType, FInventoryItem> Result;
	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		Result.Add(Slot.SlotType, Slot.Item);
	}
	return Result;
}

FInventoryItem UInventoryComponent::GetEquippedItem(EEquipmentSlotType SlotType) const
{
	int32 Index = FindEquippedSlotIndex(SlotType);
	if (Index != INDEX_NONE)
	{
		return EquippedSlots[Index].Item;
	}
	return FInventoryItem();
}

void UInventoryComponent::SwitchWeaponSlot(int Index)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerSwitchWeaponSlot(Index);
		return;
	}

	int minIndex = static_cast<int>(EEquipmentSlotType::Weapon1);
	int currentIndex = static_cast<int>(CurrentWeaponSlot);
	int normalizedIndex = currentIndex - minIndex;

	normalizedIndex = (normalizedIndex + Index) % 4;
	if (normalizedIndex < 0) normalizedIndex += 4;

	EEquipmentSlotType newWeaponSlot = static_cast<EEquipmentSlotType>(normalizedIndex + minIndex);

	if (CurrentWeaponSlot != newWeaponSlot)
	{
		CurrentWeaponSlot = newWeaponSlot;

		// StatBonusComponent에 현재 무기 슬롯 업데이트
		if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
		{
			m_PlayerState->m_StatBonusComponent->SetCurrentWeaponSlot(CurrentWeaponSlot);
		}

		// 무기 변경 이벤트 호출
		OnWeaponChanged.Broadcast(CurrentWeaponSlot, GetEquippedItem(CurrentWeaponSlot).ItemID);
	}
}

bool UInventoryComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
	// 권한 체크
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerSwapItems(FromIndex, ToIndex);
		return false;
	}
	
	// 인덱스 유효성 검사
	if (FromIndex < 0 || FromIndex >= InventoryItems.Num() ||
		ToIndex < 0 || ToIndex >= InventoryItems.Num())
		return false;

	// 같은 인덱스인 경우 무시
	if (FromIndex == ToIndex)
		return true;

	// 아이템 스왑
	InventoryItems.Swap(FromIndex, ToIndex);

	// 이벤트 발생
	BroadcastInventoryChanged();

	return true;
}

FItemData* UInventoryComponent::GetCurrentWeaponData()
{
	FInventoryItem CurrentWeapon = GetEquippedItem(CurrentWeaponSlot);
	return GetItemData(CurrentWeapon.ItemID);
}

// Blueprint 헬퍼 함수들
FItemData UInventoryComponent::GetEquippedWeaponData(EEquipmentSlotType SlotType) const
{
	FInventoryItem Item = GetEquippedItem(SlotType);
	if (FItemData* Data = GetItemData(Item.ItemID))
	{
		return *Data;
	}
	return FItemData();
}

void UInventoryComponent::GetCurrentWeaponInfo(bool& bHasWeapon, FItemData& OutWeaponData) const
{
	FInventoryItem CurrentWeapon = GetEquippedItem(CurrentWeaponSlot);
	bHasWeapon = (CurrentWeapon.ItemID > 0);
	
	if (bHasWeapon)
	{
		if (FItemData* Data = GetItemData(CurrentWeapon.ItemID))
		{
			OutWeaponData = *Data;
		}
	}
}

ASonheimPlayer* UInventoryComponent::GetSonheimPlayer()
{
	if (!m_Player && m_PlayerState)
	{
		m_Player = m_PlayerState->GetSonheimPlayer();
	}
	return m_Player;
}

// 내부 헬퍼 함수들
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
	if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
	{
		m_PlayerState->m_StatBonusComponent->ApplyItemStatBonuses(ItemID, bEquipping);
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

EEquipmentSlotType UInventoryComponent::FindEmptySlotForType(EEquipmentKindType ItemType)
{
	switch (ItemType)
	{
	case EEquipmentKindType::Head:
		return EEquipmentSlotType::Head;

	case EEquipmentKindType::Body:
		return EEquipmentSlotType::Body;

	case EEquipmentKindType::Weapon:
		// 무기 슬롯 4개 중 빈 슬롯 찾기
		if (GetEquippedItem(EEquipmentSlotType::Weapon1).IsEmpty())
			return EEquipmentSlotType::Weapon1;
		if (GetEquippedItem(EEquipmentSlotType::Weapon2).IsEmpty())
			return EEquipmentSlotType::Weapon2;
		if (GetEquippedItem(EEquipmentSlotType::Weapon3).IsEmpty())
			return EEquipmentSlotType::Weapon3;
		if (GetEquippedItem(EEquipmentSlotType::Weapon4).IsEmpty())
			return EEquipmentSlotType::Weapon4;
		// 모두 차있으면 첫 번째 슬롯 반환
		return EEquipmentSlotType::Weapon1;

	case EEquipmentKindType::Accessory:
		// 악세서리 슬롯 2개 중 빈 슬롯 찾기
		if (GetEquippedItem(EEquipmentSlotType::Accessory1).IsEmpty())
			return EEquipmentSlotType::Accessory1;
		if (GetEquippedItem(EEquipmentSlotType::Accessory2).IsEmpty())
			return EEquipmentSlotType::Accessory2;
		return EEquipmentSlotType::None;

	case EEquipmentKindType::Glider:
		return EEquipmentSlotType::Glider;

	case EEquipmentKindType::Shield:
		return EEquipmentSlotType::Shield;

	case EEquipmentKindType::SphereModule:
		return EEquipmentSlotType::SphereModule;

	default:
		return EEquipmentSlotType::None;
	}
}

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast(InventoryItems);
}

// 장착 슬롯 헬퍼 함수들
int32 UInventoryComponent::FindEquippedSlotIndex(EEquipmentSlotType SlotType) const
{
	for (int32 i = 0; i < EquippedSlots.Num(); i++)
	{
		if (EquippedSlots[i].SlotType == SlotType)
			return i;
	}
	return INDEX_NONE;
}

FInventoryItem* UInventoryComponent::GetEquippedSlotItem(EEquipmentSlotType SlotType)
{
	int32 Index = FindEquippedSlotIndex(SlotType);
	if (Index != INDEX_NONE)
	{
		return &EquippedSlots[Index].Item;
	}
	return nullptr;
}

void UInventoryComponent::SetEquippedSlot(EEquipmentSlotType SlotType, const FInventoryItem& Item)
{
	int32 Index = FindEquippedSlotIndex(SlotType);
	if (Index != INDEX_NONE)
	{
		EquippedSlots[Index].Item = Item;
	}
}

void UInventoryComponent::RemoveEquippedSlot(EEquipmentSlotType SlotType)
{
	SetEquippedSlot(SlotType, FInventoryItem());
}