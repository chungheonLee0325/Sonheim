// ContainerComponent.h
//
// [컨테이너 복제 원칙]
// - FastArray(FFastArraySerializer)로 슬롯 단위 델타 복제를 수행합니다.
//   (추가/삭제/수정/스왑 시 변경된 엔트리만 Dirty 처리)
// - 구독형 복제: 열람 중인 플레이어가 있을 때만 컨테이너 리스트를 복제합니다.
//   (PreReplication에서 Subscribers 유무로 활성/비활성 제어)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ContainerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContainerInventoryChanged, const TArray<FInventoryItem>&, Items);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerItemAdded, int32, ItemID, int32, Count);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerItemRemoved, int32, ItemID, int32, Count);

USTRUCT(BlueprintType)
struct FRepContainerEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	int32 ItemID = 0;

	UPROPERTY()
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRepContainerList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRepContainerEntry> Items;

	UPROPERTY(NotReplicated)
	class UContainerComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRepContainerEntry, FRepContainerList>(
			Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FRepContainerList> : public TStructOpsTypeTraitsBase2<FRepContainerList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UContainerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void BeginPlay() override;

	// 컨테이너 초기화
	UFUNCTION(BlueprintCallable, Category = "Container")
	void InitializeContainer(int32 InContainerID);

	// 아이템 관리 함수들
	UFUNCTION(BlueprintCallable, Category = "Container")
	bool AddItem(int32 ItemID, int32 ItemCount);

	UFUNCTION(BlueprintCallable, Category = "Container")
	bool RemoveItem(int32 ItemID, int32 ItemCount);

	UFUNCTION(BlueprintCallable, Category = "Container")
	bool RemoveItemByIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Container")
	bool SwapItems(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category = "Container")
	TArray<FInventoryItem> GetContainerInventory() const { return ContainerItems; }

	UFUNCTION(BlueprintCallable, Category = "Container")
	int32 GetMaxSlots() const { return MaxSlots; }

	// 정확한 위치 배치를 위한 유틸 (서버 전용 사용 권장)
	// 비어있지 않은 유효 인덱스에 아이템을 교체 배치
	bool SetItemAtIndex(int32 Index, const FInventoryItem& Item);
	// 현재 아이템 배열에 지정 위치로 삽입(우측으로 시프트). Index는 [0..Num] 범위만 허용
	bool InsertItemAtIndex(int32 Index, const FInventoryItem& Item);

	UFUNCTION(BlueprintCallable, Category = "Container")
	bool HasItem(int32 ItemID, int32 RequiredCount = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Container")
	int32 GetItemCount(int32 ItemID) const;

	// 서버 RPC
	UFUNCTION(Server, Reliable)
	void ServerAddItem(int32 ItemID, int32 ItemCount);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 ItemID, int32 ItemCount);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItemByIndex(int32 Index);

	UFUNCTION(Server, Reliable)
	void ServerSwapItems(int32 FromIndex, int32 ToIndex);

	// View subscription (open/close container)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Container|Net")
	void ServerSubscribeViewer(APlayerController* Viewer);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Container|Net")
	void ServerUnsubscribeViewer(APlayerController* Viewer);

	UFUNCTION(BlueprintCallable, Category = "Container")
	void SetMaxSlots(int32 NewMaxSlots) { MaxSlots = NewMaxSlots; }

	// 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnContainerInventoryChanged OnContainerInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnContainerItemAdded OnContainerItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnContainerItemRemoved OnContainerItemRemoved;

protected:
	// FastArray replicated entries
	UPROPERTY(ReplicatedUsing=OnRep_RepContainerItems)
	FRepContainerList RepContainerItems;

	// Local mirror for UI/logic; rebuilt on clients
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container")
	TArray<FInventoryItem> ContainerItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container", Replicated)
	int32 MaxSlots = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container", Replicated)
	int32 MaxStackSize = 999;

	UFUNCTION()
	void OnRep_RepContainerItems();

private:
	UPROPERTY()
	class USonheimGameInstance* GameInstance;

	FContainerData* ContainerData;

	// 헬퍼 함수들
	int32 FindItemIndex(int32 ItemID) const;
	bool IsValidItemID(int32 ItemID) const;
	FItemData* GetItemData(int32 ItemID) const;
	void BroadcastInventoryChanged();

	// Rebuild local mirror array from FastArray (sorted by SlotIndex)
	void RebuildContainerArrayFromRep();
	// Rebuild FastArray from local array (server only)
	void SyncRepFromContainerArray();

	// Fine-grained FastArray helpers (server only)
	void UpdateRepEntryAtIndex(int32 Index);
	void InsertRepEntryAtIndex(int32 Index, const FInventoryItem& Item);
	void RemoveRepEntryAtIndex(int32 Index);

	// Active viewers (server only). When empty, container list will not replicate.
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<APlayerController>> Subscribers;
};
