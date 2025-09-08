// ContainerComponent.cpp
#include "ContainerComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/Utilities/Net/FastArraySlotUtils.h"

UContainerComponent::UContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UContainerComponent, RepContainerItems);
	DOREPLIFETIME(UContainerComponent, MaxSlots);
	DOREPLIFETIME(UContainerComponent, MaxStackSize);
}

void UContainerComponent::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
	const bool bActive = Subscribers.Num() > 0;
	DOREPLIFETIME_ACTIVE_OVERRIDE(UContainerComponent, RepContainerItems, bActive);
}

void UContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
	RepContainerItems.Owner = this;

	if (GetOwnerRole() == ROLE_Authority)
	{
		// 블루프린트 등에서 미리 채워진 아이템이 있을 경우,
		// 복제 배열(RepContainerItems)과 동기화하여 클라이언트에 초기 상태를 전송합니다.
		SyncRepFromContainerArray();
	}
}

void UContainerComponent::InitializeContainer(int32 InContainerID)
{
	if (GameInstance)
	{
		ContainerData = GameInstance->GetDataContainer(InContainerID);
		if (ContainerData)
		{
			MaxSlots = ContainerData->SlotCount;
		}
	}
}

bool UContainerComponent::AddItem(int32 ItemID, int32 ItemCount)
{
	if (!IsValidItemID(ItemID) || ItemCount <= 0)
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		FItemData* ItemData = GetItemData(ItemID);
		if (!ItemData)
			return false;

		// 스택 가능한 아이템인 경우 기존 스택에 추가(엔트리 단위 델타)
		if (ItemData->bStackable)
		{
			int32 ExistingIndex = FindItemIndex(ItemID);
			if (ExistingIndex != INDEX_NONE)
			{
				int32 NewCount = ContainerItems[ExistingIndex].Count + ItemCount;
				if (NewCount <= MaxStackSize)
				{
					ContainerItems[ExistingIndex].Count = NewCount;
					UpdateRepEntryAtIndex(ExistingIndex);
					OnContainerItemAdded.Broadcast(ItemID, ItemCount);
					BroadcastInventoryChanged();
					return true;
				}
			}
		}

		// 새 슬롯에 추가(엔트리 단위 델타)
		if (ContainerItems.Num() < MaxSlots)
		{
			ContainerItems.Add(FInventoryItem(ItemID, ItemCount));
			InsertRepEntryAtIndex(ContainerItems.Num() - 1, ContainerItems.Last());
			OnContainerItemAdded.Broadcast(ItemID, ItemCount);
			BroadcastInventoryChanged();
			return true;
		}

		return false;
	}
	else
	{
		ServerAddItem(ItemID, ItemCount);
		return true;
	}
}

bool UContainerComponent::RemoveItem(int32 ItemID, int32 ItemCount)
{
	if (!IsValidItemID(ItemID) || ItemCount <= 0)
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		int32 ItemIndex = FindItemIndex(ItemID);
		if (ItemIndex == INDEX_NONE)
			return false;

		if (ContainerItems[ItemIndex].Count < ItemCount)
			return false;

		ContainerItems[ItemIndex].Count -= ItemCount;
		if (ContainerItems[ItemIndex].Count <= 0)
		{
			ContainerItems.RemoveAt(ItemIndex);
			RemoveRepEntryAtIndex(ItemIndex);
		}
		else
		{
			UpdateRepEntryAtIndex(ItemIndex);
		}
		OnContainerItemRemoved.Broadcast(ItemID, ItemCount);
		BroadcastInventoryChanged();
		return true;
	}
	else
	{
		ServerRemoveItem(ItemID, ItemCount);
		return true;
	}
}

bool UContainerComponent::RemoveItemByIndex(int32 Index)
{
	if (Index < 0 || Index >= ContainerItems.Num())
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		int32 ItemID = ContainerItems[Index].ItemID;
		int32 Count = ContainerItems[Index].Count;

		ContainerItems.RemoveAt(Index);
		RemoveRepEntryAtIndex(Index);

		OnContainerItemRemoved.Broadcast(ItemID, Count);
		BroadcastInventoryChanged();
		return true;
	}
	else
	{
		ServerRemoveItemByIndex(Index);
		return true;
	}
}

bool UContainerComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
	if (FromIndex < 0 || FromIndex >= ContainerItems.Num() ||
		ToIndex < 0 || ToIndex >= ContainerItems.Num() ||
		FromIndex == ToIndex)
		return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		ContainerItems.Swap(FromIndex, ToIndex);
		// Replicate swap by swapping payload and Dirty mark
		if (RepContainerItems.Items.IsValidIndex(FromIndex) && RepContainerItems.Items.IsValidIndex(ToIndex))
		{
			FRepContainerEntry& A = RepContainerItems.Items[FromIndex];
			FRepContainerEntry& B = RepContainerItems.Items[ToIndex];
			Swap(A.ItemID, B.ItemID);
			Swap(A.Count, B.Count);
			RepContainerItems.MarkItemDirty(A);
			RepContainerItems.MarkItemDirty(B);
		}
		BroadcastInventoryChanged();
		return true;
	}
	else
	{
		ServerSwapItems(FromIndex, ToIndex);
		return true;
	}
}

bool UContainerComponent::HasItem(int32 ItemID, int32 RequiredCount) const
{
	int32 ItemIndex = FindItemIndex(ItemID);
	if (ItemIndex == INDEX_NONE)
		return false;

	return ContainerItems[ItemIndex].Count >= RequiredCount;
}

int32 UContainerComponent::GetItemCount(int32 ItemID) const
{
	int32 ItemIndex = FindItemIndex(ItemID);
	if (ItemIndex == INDEX_NONE)
		return 0;

	return ContainerItems[ItemIndex].Count;
}

// 서버 RPC 구현
void UContainerComponent::ServerAddItem_Implementation(int32 ItemID, int32 ItemCount)
{
	AddItem(ItemID, ItemCount);
}

void UContainerComponent::ServerRemoveItem_Implementation(int32 ItemID, int32 ItemCount)
{
	RemoveItem(ItemID, ItemCount);
}

void UContainerComponent::ServerRemoveItemByIndex_Implementation(int32 Index)
{
	RemoveItemByIndex(Index);
}

void UContainerComponent::ServerSwapItems_Implementation(int32 FromIndex, int32 ToIndex)
{
	SwapItems(FromIndex, ToIndex);
}

void UContainerComponent::ServerSubscribeViewer_Implementation(APlayerController* Viewer)
{
	if (!Viewer) return;
	Subscribers.Add(Viewer);
	// Wake up replication
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UContainerComponent::ServerUnsubscribeViewer_Implementation(APlayerController* Viewer)
{
	if (!Viewer) return;
	Subscribers.Remove(Viewer);
}

// 헬퍼 함수들
int32 UContainerComponent::FindItemIndex(int32 ItemID) const
{
	for (int32 i = 0; i < ContainerItems.Num(); i++)
	{
		if (ContainerItems[i].ItemID == ItemID)
			return i;
	}
	return INDEX_NONE;
}

bool UContainerComponent::IsValidItemID(int32 ItemID) const
{
	return GameInstance && GameInstance->GetDataItem(ItemID) != nullptr;
}

FItemData* UContainerComponent::GetItemData(int32 ItemID) const
{
	return GameInstance ? GameInstance->GetDataItem(ItemID) : nullptr;
}

void UContainerComponent::BroadcastInventoryChanged()
{
	OnContainerInventoryChanged.Broadcast(ContainerItems);
}

bool UContainerComponent::SetItemAtIndex(int32 Index, const FInventoryItem& Item)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		// 서버 전용
		return false;
	}
	if (Index < 0 || Index >= ContainerItems.Num())
		return false;

	ContainerItems[Index] = Item;
	UpdateRepEntryAtIndex(Index);
	BroadcastInventoryChanged();
	return true;
}

bool UContainerComponent::InsertItemAtIndex(int32 Index, const FInventoryItem& Item)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		// 서버 전용
		return false;
	}
	if (ContainerItems.Num() >= MaxSlots)
		return false;

	// 허용 범위 [0..Num]
	if (Index < 0) Index = 0;
	if (Index > ContainerItems.Num()) Index = ContainerItems.Num();

	ContainerItems.Insert(Item, Index);
	InsertRepEntryAtIndex(Index, Item);
	BroadcastInventoryChanged();
	return true;
}

void UContainerComponent::OnRep_RepContainerItems()
{
	RebuildContainerArrayFromRep();
	BroadcastInventoryChanged();
}

void UContainerComponent::RebuildContainerArrayFromRep()
{
	ContainerItems.Empty();
	// Build sorted by SlotIndex
	TArray<FRepContainerEntry> Copy = RepContainerItems.Items;
	Copy.Sort([](const FRepContainerEntry& A, const FRepContainerEntry& B) { return A.SlotIndex < B.SlotIndex; });
	for (const FRepContainerEntry& E : Copy)
	{
		ContainerItems.Add(FInventoryItem(E.ItemID, E.Count));
	}
}

void UContainerComponent::SyncRepFromContainerArray()
{
	if (GetOwnerRole() != ROLE_Authority) return;
	RepContainerItems.Items.Empty();
	RepContainerItems.MarkArrayDirty();
	for (int32 i = 0; i < ContainerItems.Num(); ++i)
	{
		FRepContainerEntry& Entry = RepContainerItems.Items.AddDefaulted_GetRef();
		Entry.SlotIndex = i;
		Entry.ItemID = ContainerItems[i].ItemID;
		Entry.Count = ContainerItems[i].Count;
		RepContainerItems.MarkItemDirty(Entry);
	}
}

void UContainerComponent::UpdateRepEntryAtIndex(int32 Index)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	if (!ContainerItems.IsValidIndex(Index)) return;
	Sonheim::FastArray::UpdateAt(RepContainerItems, Index, ContainerItems[Index].ItemID, ContainerItems[Index].Count);
}

void UContainerComponent::InsertRepEntryAtIndex(int32 Index, const FInventoryItem& Item)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	Sonheim::FastArray::InsertAt(RepContainerItems, Index, Item.ItemID, Item.Count);
}

void UContainerComponent::RemoveRepEntryAtIndex(int32 Index)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	Sonheim::FastArray::RemoveAt(RepContainerItems, Index);
}