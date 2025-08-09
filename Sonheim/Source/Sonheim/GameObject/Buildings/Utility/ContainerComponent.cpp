// ContainerComponent.cpp
#include "ContainerComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Net/UnrealNetwork.h"

UContainerComponent::UContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UContainerComponent, ContainerItems);
	DOREPLIFETIME(UContainerComponent, MaxSlots);
	DOREPLIFETIME(UContainerComponent, MaxStackSize);
}

void UContainerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	GameInstance = Cast<USonheimGameInstance>(GetWorld()->GetGameInstance());
}

void UContainerComponent::InitializeContainer(int32 InContainerID)
{
	ContainerID = InContainerID;
	
	if (GameInstance)
	{
		ContainerData = GameInstance->GetDataContainer(ContainerID);
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

		// 스택 가능한 아이템인 경우 기존 스택에 추가
		if (ItemData->bStackable)
		{
			int32 ExistingIndex = FindItemIndex(ItemID);
			if (ExistingIndex != INDEX_NONE)
			{
				int32 NewCount = ContainerItems[ExistingIndex].Count + ItemCount;
				if (NewCount <= MaxStackSize)
				{
					ContainerItems[ExistingIndex].Count = NewCount;
					OnContainerItemAdded.Broadcast(ItemID, ItemCount);
					BroadcastInventoryChanged();
					return true;
				}
			}
		}

		// 새 슬롯에 추가
		if (ContainerItems.Num() < MaxSlots)
		{
			ContainerItems.Add(FInventoryItem(ItemID, ItemCount));
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

void UContainerComponent::OnRep_ContainerItems()
{
	BroadcastInventoryChanged();
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