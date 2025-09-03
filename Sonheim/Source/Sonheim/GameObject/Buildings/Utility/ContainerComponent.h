#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Net/UnrealNetwork.h"
#include "ContainerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContainerInventoryChanged, const TArray<FInventoryItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerItemAdded, int32, ItemID, int32, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerItemRemoved, int32, ItemID, int32, Count);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UContainerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container", ReplicatedUsing = OnRep_ContainerItems)
	TArray<FInventoryItem> ContainerItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container", Replicated)
	int32 MaxSlots = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container", Replicated)
	int32 MaxStackSize = 999;

	UFUNCTION()
	void OnRep_ContainerItems();

private:
	UPROPERTY()
	class USonheimGameInstance* GameInstance;

	FContainerData* ContainerData;

	// 헬퍼 함수들
	int32 FindItemIndex(int32 ItemID) const;
	bool IsValidItemID(int32 ItemID) const;
	FItemData* GetItemData(int32 ItemID) const;
	void BroadcastInventoryChanged();
};