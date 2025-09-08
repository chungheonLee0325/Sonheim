// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComponent.h"
#include "Sonheim/AreaObject/Attribute/StatBonusComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/Utilities/SonheimUtility.h"
#include "Sonheim/Utilities/InventoryRulesLibrary.h"
#include "Sonheim/Utilities/Net/FastArraySlotUtils.h"
#include "Sonheim/AreaObject/Skill/SonheimSkillComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 조건부 복제로 네트워크 트래픽 감소
	DOREPLIFETIME_CONDITION(UInventoryComponent, RepItems, COND_OwnerOnly);
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

	// FastArray owner
	RepItems.Owner = this;
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

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// OnRep 함수들
void UInventoryComponent::OnRep_RepItems()
{
	RebuildInventoryArrayFromRep();
	BroadcastInventoryChanged();
}

void UInventoryComponent::OnRep_EquippedItems()
{
	// 장착 아이템 변경 이벤트 호출
	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		OnEquipmentChanged.Broadcast(Slot.SlotType, Slot.Item);
	}

	// 현재 활성 무기 슬롯에 아이템이 장착/해제되었을 수 있으므로,
	// OnWeaponChanged 이벤트를 발생시켜 활성 무기 UI를 갱신합니다.
	// OnRep_CurrentWeaponSlot은 슬롯 '인덱스'가 바뀔 때만 호출되므로 이 처리가 필요합니다.
	NotifyWeaponSlot(CurrentWeaponSlot);
}

void UInventoryComponent::OnRep_CurrentWeaponSlot()
{
	// 클라이언트: 현 슬롯의 무기 UI만 동기화
	NotifyWeaponSlot(CurrentWeaponSlot);
}

void UInventoryComponent::RebuildInventoryArrayFromRep()
{
	InventoryItems.Empty();
	TArray<FRepInventoryEntry> Copy = RepItems.Items;
	Copy.Sort([](const FRepInventoryEntry& A, const FRepInventoryEntry& B) { return A.SlotIndex < B.SlotIndex; });
	for (const FRepInventoryEntry& E : Copy)
	{
		InventoryItems.Add(FInventoryItem(E.ItemID, E.Count));
	}
}

void UInventoryComponent::SyncRepFromInventoryArray()
{
	if (GetOwnerRole() != ROLE_Authority) return;
	RepItems.Items.Empty();
	RepItems.MarkArrayDirty();
	for (int32 i = 0; i < InventoryItems.Num(); ++i)
	{
		FRepInventoryEntry& Entry = RepItems.Items.AddDefaulted_GetRef();
		Entry.SlotIndex = i;
		Entry.ItemID = InventoryItems[i].ItemID;
		Entry.Count = InventoryItems[i].Count;
		RepItems.MarkItemDirty(Entry);
	}
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

void UInventoryComponent::ServerEquipItemToSlotByIndex_Implementation(int32 InventoryIndex, EEquipmentSlotType SlotType)
{
	EquipItemToSlotByIndex(InventoryIndex, SlotType);
}

void UInventoryComponent::ServerEquipItemByIndex_Implementation(int32 InventoryIndex)
{
	EquipItemByIndex(InventoryIndex);
}

void UInventoryComponent::ServerEquipItemToSlotByItemID_Implementation(int32 ItemID, EEquipmentSlotType SlotType)
{
	EquipItemToSlotByItemID(ItemID, SlotType);
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

void UInventoryComponent::ServerSwapEquippedItems_Implementation(EEquipmentSlotType SlotA, EEquipmentSlotType SlotB)
{
	SwapEquippedItems(SlotA, SlotB);
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

void UInventoryComponent::ServerDropItemByIndex_Implementation(int32 Index, int32 Count)
{
	DropItemByIndex(Index, Count);
}

void UInventoryComponent::ServerDiscardItemByIndex_Implementation(int32 Index, int32 Count)
{
	DiscardItemByIndex(Index, Count);
}

// IsDirectAcquisition 은 직접 얻은 아이템(장비 해제로 인해 인벤으로 돌아온 아이템 x)
bool UInventoryComponent::AddItem(int ItemID, int ItemCount, bool IsDirectAcquisition)
{
	// 데이터 유효성 검증
	if (!ValidateItemOperation(ItemID, ItemCount))
		return false;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		// 서버는 바로 실제 처리
		FItemData* ItemData = GetItemData(ItemID);
		if (!ItemData)
			return false;

		const bool bStackable = ItemData->bStackable;
		int32 Remaining = ItemCount;
		int32 AddedToInventory = 0;

		// 1) 자동 장착 시도 (장비/무기 & 비스택, 직접 획득한 경우에만)
		if (bAutoEquipOnPickup && !bStackable && IsDirectAcquisition)
		{
			const bool bEquipCategory =
				(ItemData->ItemCategory == EItemCategory::Equipment) ||
				(ItemData->ItemCategory == EItemCategory::Weapon);

			if (bEquipCategory)
			{
				const EEquipmentSlotType TargetSlot =
					FindEquipSlotByEquipKindType(ItemData->EquipmentData.EquipKind);

				if (GetEquippedItem(TargetSlot).IsEmpty())
				{
					if (InventoryItems.Num() < MaxInventorySlots)
					{
						// 임시로 1개 넣고, 바로 장착
						const int32 TempIndex = InventoryItems.Add(FInventoryItem(ItemID, 1));
						const bool bEquipped = EquipItemByIndex(TempIndex);

						if (bEquipped)
						{
							Remaining -= 1;
							// 아이템 획득 후 바로 장착하므로 따로 브로드캐스트
							OnItemAdded.Broadcast(ItemID, 1);
						}
						else
						{
							// 장착 실패 시 롤백
							InventoryItems.RemoveAt(TempIndex);
						}
					}
					else
					{
						// 임시 칸조차 못 만들면 바닥 드랍
						DropItem_ServerOnly(ItemID, 1, false);
						Remaining -= 1;
					}
				}
			}
		}

		// 2) (스택형) 기존 스택 채우기
		if (bStackable)
		{
			for (int32 i = 0; i < InventoryItems.Num() && Remaining > 0; ++i)
			{
				FInventoryItem& It = InventoryItems[i];
				if (It.ItemID != ItemID) continue;

				const int32 Space = MaxItemStackCount - It.Count;
				if (Space <= 0) continue;

				const int32 Add = FMath::Min(Space, Remaining);
				It.Count += Add;
				Remaining -= Add;
				AddedToInventory += Add;
				UpdateRepEntryAtIndex(i);
			}
		}

		// 3) 새 슬롯 채우기 (스택형=새 스택, 비스택형=슬롯당 1)
		while (Remaining > 0 && InventoryItems.Num() < MaxInventorySlots)
		{
			if (bStackable)
			{
				const int32 Add = FMath::Min(MaxItemStackCount, Remaining);
				InventoryItems.Add(FInventoryItem(ItemID, Add));
				InsertRepEntryAtIndex(InventoryItems.Num() - 1, InventoryItems.Last());
				Remaining -= Add;
				AddedToInventory += Add;
			}
			else
			{
				InventoryItems.Add(FInventoryItem(ItemID, 1));
				InsertRepEntryAtIndex(InventoryItems.Num() - 1, InventoryItems.Last());
				Remaining -= 1;
				AddedToInventory += 1;
			}
		}

		// 4) 잔여 오버플로우는 바닥 드랍(인터랙션 Hold)
		if (Remaining > 0)
		{
			DropItem_ServerOnly(ItemID, Remaining, bStackable);
		}

		// 이벤트
		if (IsDirectAcquisition && AddedToInventory > 0)
		{
			OnItemAdded.Broadcast(ItemID, AddedToInventory);
		}
		BroadcastInventoryChanged();
		return true;
	}
	else
	{
		// 클라 예측 수행
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_AddItem(ItemID, ItemCount);
		}
		ServerAddItem(ItemID, ItemCount);
		return true;
	}
}

bool UInventoryComponent::AddItemByInventoryItem(const FInventoryItem& InventoryItem)
{
	return AddItem(InventoryItem.ItemID, InventoryItem.Count, false);
}

bool UInventoryComponent::RemoveItem(int ItemID, int ItemCount)
{
	if (!ValidateItemOperation(ItemID, ItemCount))
		return false;

	if (GetOwnerRole() != ROLE_Authority)
	{
		if (bEnableClientPrediction)
			PerformClientPrediction_RemoveItem(ItemID, ItemCount);
		ServerRemoveItem(ItemID, ItemCount);
		return true;
	}

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData) return false;
	const bool bStackable = ItemData->bStackable;

	int Available = 0;
	for (const FInventoryItem& It : InventoryItems)
	{
		if (It.ItemID == ItemID)
			Available += bStackable ? It.Count : FMath::Max(1, It.Count);
	}
	if (Available < ItemCount)
	{
		OnItemUseFailed.Broadcast(ItemID);
		return false;
	}

	int Remaining = ItemCount;
	for (int i = 0; i < InventoryItems.Num() && Remaining > 0;)
	{
		FInventoryItem& It = InventoryItems[i];
		if (It.ItemID != ItemID)
		{
			++i;
			continue;
		}

		if (bStackable)
		{
			const int Use = FMath::Min(It.Count, Remaining);
			It.Count -= Use;
			Remaining -= Use;
			if (It.Count <= 0)
			{
				InventoryItems.RemoveAt(i);
				RemoveRepEntryAtIndex(i);
				continue;
			}
			else
			{
				UpdateRepEntryAtIndex(i);
			}
			++i;
		}
		else
		{
			const int Use = FMath::Min(FMath::Max(1, It.Count), Remaining);
			Remaining -= Use;
			if (It.Count - Use <= 0)
			{
				InventoryItems.RemoveAt(i);
				RemoveRepEntryAtIndex(i);
				continue;
			}
			It.Count -= Use;
			UpdateRepEntryAtIndex(i);
			++i;
		}
	}

	OnItemRemoved.Broadcast(ItemID, ItemCount);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::RemoveItemByIndex(int Index)
{
	if (Index < 0 || Index >= InventoryItems.Num())
		return false;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		int ItemID = InventoryItems[Index].ItemID;
		int Count = InventoryItems[Index].Count;

		InventoryItems.RemoveAt(Index);
		RemoveRepEntryAtIndex(Index);

		OnItemRemoved.Broadcast(ItemID, Count);
		BroadcastInventoryChanged();
		return true;
	}
	// 클라이언트에서 실행
	else
	{
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_RemoveItemByIndex(Index);
		}

		ServerRemoveItemByIndex(Index);
		return true;
	}
}

bool UInventoryComponent::EquipItem(int ItemID)
{
	if (!IsValidItemID(ItemID))
		return false;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		FItemData* ItemData = GetItemData(ItemID);
		if (!ItemData)
			return false;
		EEquipmentSlotType EquipSlotType = FindEquipSlotByEquipKindType(ItemData->EquipmentData.EquipKind);
		if (EquipSlotType == EEquipmentSlotType::None)
			return false;
		return EquipItemToSlotByItemID(ItemID, EquipSlotType);
	}
	// 클라이언트에서 실행
	else
	{
		// 1. 예측
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_EquipItem(ItemID);
		}

		// 2. 서버 요청
		ServerEquipItem(ItemID);
		return true;
	}
}

bool UInventoryComponent::EquipItemToSlotByItemID(int32 ItemID, EEquipmentSlotType SlotType)
{
	if (ItemID <= 0 || SlotType == EEquipmentSlotType::None) return false;
	int32 Index = FindItemIndexInInventory(ItemID);
	if (Index == INDEX_NONE) return false;
	return EquipItemToSlotByIndex(Index, SlotType);
}

bool UInventoryComponent::EquipItemByIndex(int32 InventoryIndex)
{
	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num())
		return false;

	int ItemID = InventoryItems[InventoryIndex].ItemID;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		FItemData* ItemData = GetItemData(ItemID);
		if (!ItemData)
			return false;
		EEquipmentSlotType EquipSlotType = FindEquipSlotByEquipKindType(ItemData->EquipmentData.EquipKind);
		if (EquipSlotType == EEquipmentSlotType::None)
			return false;

		return EquipItemToSlotByIndex(InventoryIndex, EquipSlotType);
	}
	// 클라이언트에서 실행
	else
	{
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_EquipItem(ItemID);
		}

		ServerEquipItemByIndex(InventoryIndex);
		return true;
	}
}

bool UInventoryComponent::UnEquipItem(EEquipmentSlotType SlotType)
{
	if (SlotType == EEquipmentSlotType::None)
		return false;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (InventoryItems.Num() >= MaxInventorySlots)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot unequip: Inventory full"));
			return false;
		}

		FInventoryItem Returned;
		if (!UnEquipCore(SlotType, Returned, /*bNotifyWeapon*/(SlotType == CurrentWeaponSlot), /*bBroadcastEmpty*/true))
			return false;

		AddItemByInventoryItem(Returned);
		return true;
	}
	// 클라이언트에서 실행
	else
	{
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_UnEquipItem(SlotType);
		}

		ServerUnEquipItem(SlotType);
		return true;
	}
}

bool UInventoryComponent::HasItem(int ItemID, int RequiredCount) const
{
	if (RequiredCount <= 0 || !IsValidItemID(ItemID))
		return false;

	const FItemData* ItemData = GetItemData(ItemID);
	// 아이템 데이터를 찾을 수 없으면 유효한 아이템이 아닙니다.
	if (!ItemData) return false;
	
	const bool bStackable = ItemData->bStackable;

	int32 TotalCount = 0;
	// 인벤토리 내의 모든 스택을 확인하여 총 개수를 계산합니다.
	// 장착된 아이템은 소모 대상이 아니므로 계산에 포함하지 않습니다.
	for (const FInventoryItem& Item : InventoryItems)
	{
		if (Item.ItemID == ItemID)
		{
			TotalCount += bStackable ? Item.Count : FMath::Max(1, Item.Count);
		}
	}

	return TotalCount >= RequiredCount;
}

int UInventoryComponent::GetItemCount(int ItemID) const
{
	if (!IsValidItemID(ItemID))
	{
		return 0;
	}

	int Total = 0;

	// 아이템 정의(스택 가능 여부) 가져오기
	const FItemData* ItemData = GetItemData(ItemID);
	const bool bStackable = ItemData ? ItemData->bStackable : true;

	// 1) 인벤토리 전 칸 합산
	for (const FInventoryItem& It : InventoryItems)
	{
		if (It.ItemID == ItemID)
		{
			// 비스택형은 슬롯 개수를 세는 것이 원칙이지만,
			// Count가 0/1 이외로 들어온 케이스도 방어적으로 처리
			Total += bStackable ? It.Count : FMath::Max(1, It.Count);
		}
	}

	// 2) 장비 슬롯(착용 중)도 포함해서 합산
	for (const FEquippedSlot& Slot : EquippedSlots)
	{
		const FInventoryItem& Equipped = Slot.Item;
		if (Equipped.ItemID == ItemID)
		{
			Total += bStackable ? Equipped.Count : 1;
		}
	}

	return Total;
}

void UInventoryComponent::GetCurrentWeaponAmmoInfo(int32& OutItemID, int32& OutAmmoValue, int32& OutMagazineValue) const
{
	OutItemID = 0;
	OutAmmoValue = 0;
	OutMagazineValue = 0;

	const FItemData* WeaponData = GetCurrentWeaponData();
	if (WeaponData && WeaponData->EquipmentData.bUseBullet && WeaponData->EquipmentData.BulletItemID.Num() > 0)
	{
		OutItemID = *WeaponData->EquipmentData.BulletItemID.begin();
		OutAmmoValue = GetItemCount(OutItemID);
		// 현재는 탄창 개념이 없으므로 동일하게 설정
		OutMagazineValue = GetItemCount(OutItemID);
	}
}

int32 UInventoryComponent::GetPalSphereCount() const
{
	// ToDo : 팰스피어 종류 생기면 어떻게 UI 구성되는지 따라 구현 변경될듯
	const FInventoryItem* PalSphere = FindFirstItemByCategory(EItemCategory::Sphere);

	return PalSphere ? PalSphere->Count : 0;
}

TArray<FInventoryItem> UInventoryComponent::GetInventory() const
{
	return InventoryItems;
}

bool UInventoryComponent::IsItemCompatibleWithSlot(int32 ItemID, EEquipmentSlotType SlotType) const
{
	if (ItemID <= 0 || SlotType == EEquipmentSlotType::None) return false;
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData) return false;
	return AcceptsSlot(SlotType, *ItemData);
}

bool UInventoryComponent::SetInventoryItemAtIndex(int32 Index, const FInventoryItem& NewItem)
{
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	if (Index < 0 || Index >= InventoryItems.Num())
		return false;

	InventoryItems[Index] = NewItem;
	UpdateRepEntryAtIndex(Index);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::InsertInventoryItemAtIndex(int32 Index, const FInventoryItem& NewItem)
{
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	if (InventoryItems.Num() >= MaxInventorySlots)
		return false;
	Index = FMath::Clamp(Index, 0, InventoryItems.Num());
	InventoryItems.Insert(NewItem, Index);
	InsertRepEntryAtIndex(Index, NewItem);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::AcceptsSlot(EEquipmentSlotType SlotType, const FItemData& ItemData) const
{
	if (!(ItemData.ItemCategory == EItemCategory::Equipment || ItemData.ItemCategory == EItemCategory::Weapon))
		return false;
	return UInventoryRulesLibrary::AcceptsSlotByKind(SlotType, ItemData.EquipmentData.EquipKind);
}

void UInventoryComponent::ApplyEquipState(EEquipmentSlotType SlotType, int ItemID, bool bEquip)
{
	if (!m_PlayerState || !m_PlayerState->m_StatBonusComponent) return;
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData) return;

	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon)
	{
		m_PlayerState->m_StatBonusComponent->RegisterEquippedItem(SlotType, ItemID, bEquip);
	}
	else
	{
		ApplyEquipmentStats(ItemID, bEquip);
	}
}

void UInventoryComponent::NotifyWeaponSlot(EEquipmentSlotType SlotType)
{
	switch (SlotType)
	{
	case EEquipmentSlotType::Weapon1:
	case EEquipmentSlotType::Weapon2:
	case EEquipmentSlotType::Weapon3:
	case EEquipmentSlotType::Weapon4:
		{
			const int ItemID = GetEquippedItem(SlotType).ItemID;
			OnWeaponChanged.Broadcast(SlotType, ItemID);
			// SkillComponent에 활성 무기 스킬 그랜트 교체를 통지
			if (ASonheimPlayer* Player = GetSonheimPlayer())
			{
				if (USonheimSkillComponent* Comp = Player->GetSkillComponent())
				{
					int32 SkillId = 0;
					if (ItemID > 0 && m_GameInstance)
					{
						if (FItemData* ItemData = m_GameInstance->GetDataItem(ItemID))
						{
							SkillId = ItemData->EquipmentData.SkillID;
						}
					}
					TArray<int32> Skills;
					if (SkillId > 0) { Skills.Add(SkillId); }
					Comp->ReplaceGrant(ActiveWeaponGrantId, Skills);
				}
			}
			break;
		}
	default:
		break;
	}
}

bool UInventoryComponent::UnEquipCore(EEquipmentSlotType SlotType, FInventoryItem& OutReturned, bool bNotifyWeapon,
                                      bool bBroadcastEmpty)
{
	OutReturned = FInventoryItem();
	FInventoryItem* EquippedItem = GetEquippedSlotItem(SlotType);
	if (!EquippedItem || EquippedItem->ItemID == 0)
		return false;

	const int ItemID = EquippedItem->ItemID;
	OutReturned = *EquippedItem;
	OutReturned.bIsEquipped = false;

	// 스탯 제거
	ApplyEquipState(SlotType, ItemID, false);

	// 슬롯 비우기
	RemoveEquippedSlot(SlotType);

	if (bBroadcastEmpty)
	{
		OnEquipmentChanged.Broadcast(SlotType, FInventoryItem());
	}

	// 무기 슬롯을 해제했고, 현재 활성 슬롯이었다면 활성 슬롯을 다음 장착 무기로 이동
	const bool bIsWeaponSlot = (SlotType == EEquipmentSlotType::Weapon1 || SlotType == EEquipmentSlotType::Weapon2 ||
		SlotType == EEquipmentSlotType::Weapon3 || SlotType == EEquipmentSlotType::Weapon4);
	if (bNotifyWeapon)
	{
		if (bIsWeaponSlot && SlotType == CurrentWeaponSlot)
		{
			// 다음 사용 가능한 무기 슬롯 탐색 (순환)
			EEquipmentSlotType WeaponSlots[4] = {
				EEquipmentSlotType::Weapon1, EEquipmentSlotType::Weapon2,
				EEquipmentSlotType::Weapon3, EEquipmentSlotType::Weapon4
			};
			int curIdx = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (WeaponSlots[i] == SlotType)
				{
					curIdx = i;
					break;
				}
			}

			EEquipmentSlotType NewSlot = SlotType; // 기본은 그대로
			for (int step = 1; step <= 4; ++step)
			{
				int idx = (curIdx + step) % 4;
				if (!GetEquippedItem(WeaponSlots[idx]).IsEmpty())
				{
					NewSlot = WeaponSlots[idx];
					break;
				}
			}

			// 활성 슬롯 변경 및 스탯 롤오버
			// 선택 슬롯은 자동 전환하지 않음. 현재 슬롯 상태만 알림(빈칸=0)
			NotifyWeaponSlot(SlotType);
		}
		// 비활성 슬롯이면 무기 선택 신호를 보내지 않음 (UI는 OnEquipmentChanged로만 갱신)
	}
	return true;
}

void UInventoryComponent::EquipCore(EEquipmentSlotType SlotType, const FInventoryItem& NewItem, bool bNotifyWeapon)
{
	// 슬롯에 장착
	FInventoryItem Item = NewItem;
	Item.bIsEquipped = true;
	SetEquippedSlot(SlotType, Item);

	// 스탯 적용
	ApplyEquipState(SlotType, Item.ItemID, true);

	// 이벤트
	OnEquipmentChanged.Broadcast(SlotType, Item);
	if (bNotifyWeapon)
	{
		NotifyWeaponSlot(SlotType);
	}
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
	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		int minIndex = static_cast<int>(EEquipmentSlotType::Weapon1);
		int currentIndex = static_cast<int>(CurrentWeaponSlot);
		int normalizedIndex = currentIndex - minIndex;

		normalizedIndex = (normalizedIndex + Index) % 4;
		if (normalizedIndex < 0) normalizedIndex += 4;

		EEquipmentSlotType newWeaponSlot = static_cast<EEquipmentSlotType>(normalizedIndex + minIndex);

		if (CurrentWeaponSlot != newWeaponSlot)
		{
			CurrentWeaponSlot = newWeaponSlot;

			if (m_PlayerState && m_PlayerState->m_StatBonusComponent)
			{
				m_PlayerState->m_StatBonusComponent->SetCurrentWeaponSlot(CurrentWeaponSlot);
			}

			NotifyWeaponSlot(CurrentWeaponSlot);

			GetOwner()->ForceNetUpdate();
		}
	}
	// 클라이언트에서 실행
	else
	{
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_SwitchWeaponSlot(Index);
		}

		ServerSwitchWeaponSlot(Index);
	}
}

bool UInventoryComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
	if (FromIndex < 0 || FromIndex >= InventoryItems.Num() ||
		ToIndex < 0 || ToIndex >= InventoryItems.Num() ||
		FromIndex == ToIndex)
		return false;

	// 서버에서 실행
	if (GetOwnerRole() == ROLE_Authority)
	{
		InventoryItems.Swap(FromIndex, ToIndex);
		UpdateRepEntryAtIndex(FromIndex);
		UpdateRepEntryAtIndex(ToIndex);
		BroadcastInventoryChanged();
		return true;
	}
	// 클라이언트에서 실행
	else
	{
		// 1. 예측
		if (bEnableClientPrediction)
		{
			PerformClientPrediction_SwapItems(FromIndex, ToIndex);
		}

		// 2. 서버 요청
		ServerSwapItems(FromIndex, ToIndex);
		return true;
	}
}

void UInventoryComponent::DropItem_ServerOnly(int32 ItemID, int32 Count, bool bStackable)
{
	if (!ensure(GetOwnerRole() == ROLE_Authority) || Count <= 0) return;

	// 드랍 위치: 플레이어 앞(없으면 Owner 위치)
	FVector DropAt = FVector::ZeroVector;
	AActor* OwnerActor = Cast<AActor>(GetOwner());


	if (m_PlayerState)
	{
		if (m_PlayerState->GetSonheimPlayer())
		{
			DropAt = m_PlayerState->GetSonheimPlayer()->GetActorLocation()
				+ m_PlayerState->GetSonheimPlayer()->GetActorForwardVector() * 150.f
				+ FVector(0, 0, 50.f);
		}
	}
	else if (OwnerActor)
	{
		DropAt = OwnerActor->GetActorLocation() + FVector(0, 0, 40.f);
	}

	// 홀드 인터랙션 필요로 드랍
	FItemSpawnOptions Opt;

	if (bStackable)
	{
		// 스택형: 한 액터에 Count 전부 담아 스폰
		Opt = MakeInteractable(Count, EItemInteractionType::Hold, 0.4f);
		USonheimUtility::SpawnItems(this, ItemID, 1, DropAt, 50.f, Opt);
	}
	else
	{
		// 비스택형: 개별 액터로 여러 개 스폰
		Opt = MakeInteractable(1, EItemInteractionType::Hold, 0.4f);
		USonheimUtility::SpawnItems(this, ItemID, Count, DropAt, 50.f, Opt);
	}
}

bool UInventoryComponent::DropItemByIndex(int32 Index, int32 Count)
{
	if (Index < 0 || Index >= InventoryItems.Num() || Count <= 0) return false;

	FInventoryItem& It = InventoryItems[Index];
	FItemData* Data = GetItemData(It.ItemID);
	if (!Data) return false;

	const bool bStack = Data->bStackable;
	const int32 Use = bStack ? FMath::Min(It.Count, Count) : 1;

	// 차감
	It.Count -= Use;
	const int32 ItemID = It.ItemID;
	if (It.Count <= 0) InventoryItems.RemoveAt(Index);

	// 월드 드랍
	DropItem_ServerOnly(ItemID, Use, bStack);

	OnItemRemoved.Broadcast(ItemID, Use);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::DiscardItemByIndex(int32 Index, int32 Count)
{
	if (Index < 0 || Index >= InventoryItems.Num() || Count <= 0) return false;

	FInventoryItem& It = InventoryItems[Index];
	FItemData* Data = GetItemData(It.ItemID);
	if (!Data) return false;

	const bool bStack = Data->bStackable;
	const int32 Use = bStack ? FMath::Min(It.Count, Count) : 1;

	It.Count -= Use;
	const int32 ItemID = It.ItemID;
	if (It.Count <= 0) InventoryItems.RemoveAt(Index);

	// 스폰 없음(완전 삭제)
	OnItemRemoved.Broadcast(ItemID, Use);
	BroadcastInventoryChanged();
	return true;
}

FItemData* UInventoryComponent::GetCurrentWeaponData() const
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

const FInventoryItem* UInventoryComponent::FindFirstItemByCategory(EItemCategory ItemCategory) const
{
	for (const FInventoryItem& Item : InventoryItems)
	{
		if (const FItemData* ItemData = GetItemData(Item.ItemID))
		{
			if (ItemData->ItemCategory == ItemCategory)
			{
				return &Item;
			}
		}
	}
	return nullptr;
}

TSet<const FInventoryItem*> UInventoryComponent::FindItemsByCategory(EItemCategory ItemCategory) const
{
	TSet<const FInventoryItem*> Result;
	for (const FInventoryItem& Item : InventoryItems)
	{
		if (const FItemData* ItemData = GetItemData(Item.ItemID))
		{
			if (ItemData->ItemCategory == ItemCategory)
			{
				Result.Add(&Item);
			}
		}
	}

	return Result;
}

bool UInventoryComponent::EquipItemDirect(int32 ItemID, EEquipmentSlotType SlotType, FInventoryItem& OutReplaced)
{
	OutReplaced = FInventoryItem();
	if (ItemID <= 0 || SlotType == EEquipmentSlotType::None) return false;

	if (GetOwnerRole() != ROLE_Authority) return false;

	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData) return false;
	if (!AcceptsSlot(SlotType, *ItemData))
		return false;

	// 기존 장비 해제(알림/빈 슬롯 브로드캐스트는 생략)
	FInventoryItem Dummy;
	UnEquipCore(SlotType, OutReplaced, /*bNotifyWeapon*/false, /*bBroadcastEmpty*/false);

	// 새 장비 장착(무기 알림: 현재 슬롯일 때만)
	FInventoryItem NewItem(ItemID, 1);
	EquipCore(SlotType, NewItem, /*bNotifyWeapon*/(SlotType == CurrentWeaponSlot));

	return true;
}

EEquipmentSlotType UInventoryComponent::FindEquipSlotByEquipKindType(EEquipmentKindType ItemKindType) const
{
	switch (ItemKindType)
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
	if (GetOwnerRole() == ROLE_Authority)
	{
		SyncRepFromInventoryArray();
		GetOwner()->ForceNetUpdate();
	}
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


// 클라이언트 예측 함수
void UInventoryComponent::PerformClientPrediction_AddItem(int ItemID, int ItemCount)
{
	// 간단한 검증
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
		return;

	// 예측된 UI 상태 생성
	TArray<FInventoryItem> PredictedInventory = InventoryItems;

	int32 AddedToInventory = 0;
	int32 Remaining = ItemCount;

	// 1) 자동 장착(서버 규칙과 유사): 비스택 + 설정 켜짐 + 빈 장비 슬롯
	bool bDidEquipOne = false;
	if (bAutoEquipOnPickup && !ItemData->bStackable && Remaining > 0)
	{
		const EEquipmentSlotType TargetSlot = FindEquipSlotByEquipKindType(ItemData->EquipmentData.EquipKind);
		if (TargetSlot != EEquipmentSlotType::None && GetEquippedItem(TargetSlot).IsEmpty())
		{
			// 예측: 해당 슬롯에 장착됨을 UI로 반영
			FInventoryItem EquipPreview(ItemID, 1);
			EquipPreview.bIsEquipped = true;
			OnEquipmentChanged.Broadcast(TargetSlot, EquipPreview);
			if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon && TargetSlot == CurrentWeaponSlot)
			{
				OnWeaponChanged.Broadcast(TargetSlot, ItemID);
			}
			bDidEquipOne = true;
			Remaining -= 1;
			OnItemAdded.Broadcast(ItemID, 1);
		}
	}

	// 2) 나머지 인벤 반영 (스택/새 슬롯)
	if (Remaining > 0)
	{
		if (ItemData->bStackable)
		{
			int ExistingItemIndex = FindItemIndexInInventory(ItemID);
			if (ExistingItemIndex != INDEX_NONE)
			{
				PredictedInventory[ExistingItemIndex].Count += Remaining;
				AddedToInventory += Remaining;
				Remaining = 0;
			}
			else if (PredictedInventory.Num() < MaxInventorySlots)
			{
				PredictedInventory.Add(FInventoryItem(ItemID, Remaining));
				AddedToInventory += Remaining;
				Remaining = 0;
			}
		}
		else
		{
			while (Remaining > 0 && PredictedInventory.Num() < MaxInventorySlots)
			{
				PredictedInventory.Add(FInventoryItem(ItemID, 1));
				AddedToInventory += 1;
				Remaining -= 1;
			}
		}
	}

	if (AddedToInventory > 0)
	{
		OnItemAdded.Broadcast(ItemID, AddedToInventory);
	}

	OnInventoryChanged.Broadcast(PredictedInventory);

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
		                                 FString::Printf(TEXT("[Client] Predicted: +%d x%d"), ItemID, ItemCount));
	}
#endif
}

void UInventoryComponent::PerformClientPrediction_EquipItem(int ItemID)
{
	FItemData* ItemData = GetItemData(ItemID);
	if (!ItemData)
		return;

	EEquipmentSlotType SlotType = FindEquipSlotByEquipKindType(ItemData->EquipmentData.EquipKind);
	if (SlotType == EEquipmentSlotType::None)
		return;

	int ItemIndex = FindItemIndexInInventory(ItemID);
	if (ItemIndex == INDEX_NONE)
		return;

	// 예측된 인벤토리 상태
	TArray<FInventoryItem> PredictedInventory = InventoryItems;
	FInventoryItem Item = PredictedInventory[ItemIndex];
	Item.bIsEquipped = true;

	// 인벤토리에서 제거
	PredictedInventory.RemoveAt(ItemIndex);

	// UI 업데이트
	OnInventoryChanged.Broadcast(PredictedInventory);
	OnEquipmentChanged.Broadcast(SlotType, Item);

	if (ItemData->EquipmentData.EquipKind == EEquipmentKindType::Weapon && SlotType == CurrentWeaponSlot)
	{
		OnWeaponChanged.Broadcast(SlotType, ItemID);
	}
}

void UInventoryComponent::PerformClientPrediction_UnEquipItem(EEquipmentSlotType SlotType)
{
	FInventoryItem* EquippedItem = GetEquippedSlotItem(SlotType);
	if (!EquippedItem || EquippedItem->ItemID == 0)
		return;

	if (InventoryItems.Num() >= MaxInventorySlots)
		return;

	// 예측된 인벤토리 상태
	TArray<FInventoryItem> PredictedInventory = InventoryItems;
	FInventoryItem ItemToReturn = *EquippedItem;
	ItemToReturn.bIsEquipped = false;

	// 인벤토리에 추가
	PredictedInventory.Add(ItemToReturn);

	// UI 업데이트
	OnInventoryChanged.Broadcast(PredictedInventory);
	OnEquipmentChanged.Broadcast(SlotType, FInventoryItem());

	// 무기 슬롯이며 현재 선택된 슬롯인 경우에만 무기 정보 갱신(예측: 0으로 표시)
	if ((SlotType == EEquipmentSlotType::Weapon1 || SlotType == EEquipmentSlotType::Weapon2 ||
			SlotType == EEquipmentSlotType::Weapon3 || SlotType == EEquipmentSlotType::Weapon4) &&
		SlotType == CurrentWeaponSlot)
	{
		OnWeaponChanged.Broadcast(SlotType, 0);
	}
}

void UInventoryComponent::PerformClientPrediction_RemoveItem(int ItemID, int ItemCount)
{
	TArray<FInventoryItem> PredictedInventory = InventoryItems;

	int ItemIndex = -1;
	for (int i = 0; i < PredictedInventory.Num(); i++)
	{
		if (PredictedInventory[i].ItemID == ItemID)
		{
			ItemIndex = i;
			break;
		}
	}

	if (ItemIndex != -1 && PredictedInventory[ItemIndex].Count >= ItemCount)
	{
		PredictedInventory[ItemIndex].Count -= ItemCount;
		if (PredictedInventory[ItemIndex].Count <= 0)
		{
			PredictedInventory.RemoveAt(ItemIndex);
		}

		OnInventoryChanged.Broadcast(PredictedInventory);
		OnItemRemoved.Broadcast(ItemID, ItemCount);
	}
}

void UInventoryComponent::PerformClientPrediction_SwapItems(int32 FromIndex, int32 ToIndex)
{
	TArray<FInventoryItem> PredictedInventory = InventoryItems;
	PredictedInventory.Swap(FromIndex, ToIndex);
	OnInventoryChanged.Broadcast(PredictedInventory);
}

void UInventoryComponent::PerformClientPrediction_RemoveItemByIndex(int Index)
{
	if (Index < 0 || Index >= InventoryItems.Num())
		return;

	TArray<FInventoryItem> PredictedInventory = InventoryItems;
	int ItemID = PredictedInventory[Index].ItemID;
	int Count = PredictedInventory[Index].Count;

	PredictedInventory.RemoveAt(Index);

	OnInventoryChanged.Broadcast(PredictedInventory);
	OnItemRemoved.Broadcast(ItemID, Count);
}

void UInventoryComponent::PerformClientPrediction_SwitchWeaponSlot(int Index)
{
	int minIndex = static_cast<int>(EEquipmentSlotType::Weapon1);
	int currentIndex = static_cast<int>(CurrentWeaponSlot);
	int normalizedIndex = currentIndex - minIndex;

	normalizedIndex = (normalizedIndex + Index) % 4;
	if (normalizedIndex < 0) normalizedIndex += 4;

	EEquipmentSlotType newWeaponSlot = static_cast<EEquipmentSlotType>(normalizedIndex + minIndex);

	if (CurrentWeaponSlot != newWeaponSlot)
	{
		// UI만 업데이트 (실제 CurrentWeaponSlot은 변경하지 않음)
		OnWeaponChanged.Broadcast(newWeaponSlot, GetEquippedItem(newWeaponSlot).ItemID);
	}
}

// 지정한 장비 슬롯으로 인벤토리 아이템 장착(스왑 지원)
bool UInventoryComponent::EquipItemToSlotByIndex(int32 InventoryIndex, EEquipmentSlotType SlotType)
{
	if (InventoryIndex < 0 || InventoryIndex >= InventoryItems.Num() || SlotType == EEquipmentSlotType::None)
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		const int ItemID = InventoryItems[InventoryIndex].ItemID;
		FItemData* ItemData = GetItemData(ItemID);
		if (!ItemData)
			return false;
		if (!AcceptsSlot(SlotType, *ItemData))
			return false;

		// 현재 장비 해제(이벤트/무기 알림 포함)
		FInventoryItem ExistingItem;
		const bool bHadExisting = UnEquipCore(SlotType, ExistingItem, /*bNotifyWeapon*/(SlotType == CurrentWeaponSlot),
		                                      /*bBroadcastEmpty*/true);

		// 새 장비 장착(무기 알림 포함)
		FInventoryItem NewEquip = InventoryItems[InventoryIndex];
		EquipCore(SlotType, NewEquip, /*bNotifyWeapon*/(SlotType == CurrentWeaponSlot));

		// 인벤토리 처리: 기존 장비가 있으면 해당 인덱스에 교체, 없으면 제거
		if (bHadExisting)
		{
			if (InventoryItems.IsValidIndex(InventoryIndex))
			{
				InventoryItems[InventoryIndex] = ExistingItem;
				BroadcastInventoryChanged();
			}
			else
			{
				// 방어적: 인덱스가 유효하지 않다면 끝에 추가
				InventoryItems.Add(ExistingItem);
				BroadcastInventoryChanged();
			}
		}
		else
		{
			RemoveItemByIndex(InventoryIndex);
		}

		return true;
	}
	else
	{
		// 클라이언트: 서버에 위임 (간단화)
		ServerEquipItemToSlotByIndex(InventoryIndex, SlotType);
		return true;
	}
}

// 장비 슬롯 간 스왑
bool UInventoryComponent::SwapEquippedItems(EEquipmentSlotType SlotA, EEquipmentSlotType SlotB)
{
	if (SlotA == EEquipmentSlotType::None || SlotB == EEquipmentSlotType::None || SlotA == SlotB)
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		const int32 IndexA = FindEquippedSlotIndex(SlotA);
		const int32 IndexB = FindEquippedSlotIndex(SlotB);
		if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
			return false;

		// 현재 장비 복사
		FInventoryItem ItemA = EquippedSlots[IndexA].Item;
		FInventoryItem ItemB = EquippedSlots[IndexB].Item;

		// 슬롯 비우기(스탯 제거, UI 빈 슬롯 브로드캐스트 생략, 무기 알림 생략)
		FInventoryItem Dummy;
		UnEquipCore(SlotA, Dummy, /*bNotifyWeapon*/false, /*bBroadcastEmpty*/false);
		UnEquipCore(SlotB, Dummy, /*bNotifyWeapon*/false, /*bBroadcastEmpty*/false);

		// 교차 장착(활성 무기 슬롯만 알림)
		EquipCore(SlotA, ItemB, /*bNotifyWeapon*/(CurrentWeaponSlot == SlotA));
		EquipCore(SlotB, ItemA, /*bNotifyWeapon*/(CurrentWeaponSlot == SlotB));

		return true;
	}
	else
	{
		ServerSwapEquippedItems(SlotA, SlotB);
		return true;
	}
}

// === FastArray entry helpers ===
void UInventoryComponent::UpdateRepEntryAtIndex(int32 Index)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	if (!InventoryItems.IsValidIndex(Index)) return;
	Sonheim::FastArray::UpdateAt(RepItems, Index, InventoryItems[Index].ItemID, InventoryItems[Index].Count);
	RepItems.MarkItemDirty(RepItems.Items[Index]);

	GetOwner()->ForceNetUpdate();
}

void UInventoryComponent::InsertRepEntryAtIndex(int32 Index, const FInventoryItem& Item)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	Sonheim::FastArray::InsertAt(RepItems, Index, Item.ItemID, Item.Count);
	RepItems.MarkArrayDirty();

	GetOwner()->ForceNetUpdate();
}

void UInventoryComponent::RemoveRepEntryAtIndex(int32 Index)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	Sonheim::FastArray::RemoveAt(RepItems, Index);
	RepItems.MarkArrayDirty();

	GetOwner()->ForceNetUpdate();
}

// 현재 아이템 전부 업데이트 로직이므로, 추후 도입예정..
// void FRepInventoryEntry::PostReplicatedAdd(const FRepInventoryList& Array)
// {
// }
//
// void FRepInventoryEntry::PreReplicatedRemove(const FRepInventoryList& Array)
// {
// }
//
// void FRepInventoryEntry::PostReplicatedChange(const FRepInventoryList& Array)
// {
// }
