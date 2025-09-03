// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetSerialization.h"
#include "InventoryComponent.generated.h"

class ASonheimPlayerState;
class USonheimGameInstance;
class ASonheimPlayer;

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const TArray<FInventoryItem>&, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlotType, Slot, FInventoryItem, InventoryItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, EEquipmentSlotType, Slot, int, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, int, ItemID, int, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, int, ItemID, int, Count);

// 장착된 아이템을 추적하기 위한 구조체
USTRUCT(BlueprintType)
struct FEquippedSlot
{
	GENERATED_BODY()

	UPROPERTY()
	EEquipmentSlotType SlotType = EEquipmentSlotType::None;

	UPROPERTY()
	FInventoryItem Item;

	FEquippedSlot() {}
	
	FEquippedSlot(EEquipmentSlotType InSlot, const FInventoryItem& InItem)
		: SlotType(InSlot), Item(InItem) {}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 인벤토리 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	int32 MaxInventorySlots = 42;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	int32 MaxItemStackCount = 9999;

	// 인벤토리 데이터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", ReplicatedUsing=OnRep_InventoryItems)
	TArray<FInventoryItem> InventoryItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", ReplicatedUsing=OnRep_EquippedItems)
	TArray<FEquippedSlot> EquippedSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Inventory", ReplicatedUsing=OnRep_CurrentWeaponSlot)
	EEquipmentSlotType CurrentWeaponSlot = EEquipmentSlotType::Weapon1;

	// OnRep 함수들
	UFUNCTION()
	void OnRep_InventoryItems();

	UFUNCTION()
	void OnRep_EquippedItems();

	UFUNCTION()
	void OnRep_CurrentWeaponSlot();

	// 서버 RPC 함수들
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerAddItem(int ItemID, int ItemCount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerRemoveItem(int ItemID, int ItemCount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerRemoveItemByIndex(int Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerEquipItem(int ItemID);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerEquipItemByIndex(int32 InventoryIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerUnEquipItem(EEquipmentSlotType SlotType);


	// 지정 장비 슬롯으로 아이템 장착 (ItemID 기반)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerEquipItemToSlotByItemID(int32 ItemID, EEquipmentSlotType SlotType);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerSwitchWeaponSlot(int Index);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerSwapItems(int32 FromIndex, int32 ToIndex);

	// 지정 슬롯으로 장비 장착(인벤토리 인덱스 기준)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerEquipItemToSlotByIndex(int32 InventoryIndex, EEquipmentSlotType SlotType);

	// 장비 슬롯 간 스왑
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerSwapEquippedItems(EEquipmentSlotType SlotA, EEquipmentSlotType SlotB);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerDropItemByIndex(int32 Index, int32 Count);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void ServerDiscardItemByIndex(int32 Index, int32 Count);

	// 인벤토리 관련 함수들
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItem(int ItemID, int ItemCount, bool IsDirectAcquisition = true);

	bool AddItemByInventoryItem(const FInventoryItem& InventoryItem);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveItem(int ItemID, int ItemCount);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveItemByIndex(int Index);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItem(int ItemID);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItemByIndex(int32 InventoryIndex);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool UnEquipItem(EEquipmentSlotType SlotType);


	// 지정 장비 슬롯으로 아이템 장착 (ItemID 기반)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItemToSlotByItemID(int32 ItemID, EEquipmentSlotType SlotType);
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool HasItem(int ItemID, int RequiredCount = 1) const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	int GetItemCount(int ItemID) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    TArray<FInventoryItem> GetInventory() const;

    // 서버에서 인벤토리 특정 인덱스의 아이템을 교체 배치
    bool SetInventoryItemAtIndex(int32 Index, const FInventoryItem& NewItem);

	// 외부(상자 등)에서 바로 장착: 인벤토리를 거치지 않고 해당 슬롯에 장착
	// OutReplaced: 기존 장착 아이템(있으면 반환, 없으면 ItemID=0)
	bool EquipItemDirect(int32 ItemID, EEquipmentSlotType SlotType, FInventoryItem& OutReplaced);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	TMap<EEquipmentSlotType, FInventoryItem> GetEquippedItems() const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	FInventoryItem GetEquippedItem(EEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SwitchWeaponSlot(int Index);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SwapItems(int32 FromIndex, int32 ToIndex);

	// 지정 슬롯으로 장비 장착(인벤토리 인덱스 기준)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItemToSlotByIndex(int32 InventoryIndex, EEquipmentSlotType SlotType);

	// 장비 슬롯 간 스왑
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SwapEquippedItems(EEquipmentSlotType SlotA, EEquipmentSlotType SlotB);

	// Item ID로 조회
	void DropItem_ServerOnly(int32 ItemID, int32 Count, bool bStackable);

	FItemData* GetCurrentWeaponData();

	// Blueprint 헬퍼 함수들
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Get Equipped Weapon Data"))
	FItemData GetEquippedWeaponData(EEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Get Current Weapon Info"))
	void GetCurrentWeaponInfo(bool& bHasWeapon, FItemData& OutWeaponData) const;

	ASonheimPlayer* GetSonheimPlayer();

	// 이벤트 델리게이트
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnItemRemoved OnItemRemoved;

private:
	// 내부 헬퍼 함수들
	bool IsValidItemID(int ItemID) const;
	FItemData* GetItemData(int ItemID) const;
	void ApplyEquipmentStats(int ItemID, bool bEquipping);
	int FindItemIndexInInventory(int ItemID) const;
	// 장비 종류로 어느 슬롯에 들어갈수 있는지 반환
	EEquipmentSlotType FindEquipSlotByEquipKindType(EEquipmentKindType ItemKindType) const;
	void BroadcastInventoryChanged();
	
	// 장착 슬롯 헬퍼 함수들
	int32 FindEquippedSlotIndex(EEquipmentSlotType SlotType) const;
	FInventoryItem* GetEquippedSlotItem(EEquipmentSlotType SlotType);
	void SetEquippedSlot(EEquipmentSlotType SlotType, const FInventoryItem& Item);
	void RemoveEquippedSlot(EEquipmentSlotType SlotType);

	// 데이터 유효성 검증
	bool ValidateItemCount(int ItemCount) const;
	bool ValidateItemOperation(int ItemID, int ItemCount) const;
	
	// 인벤토리 슬롯 인덱스로 조회 인벤 차감 + 월드에 드랍
	bool DropItemByIndex(int32 Index, int32 Count);
	// 인벤토리 슬롯 인덱스로 조회 인벤 차감(삭제)
	bool DiscardItemByIndex(int32 Index, int32 Count);

	// Client Prediction 설정
	UPROPERTY(EditDefaultsOnly, Category = "Network")
	bool bEnableClientPrediction = true;

	// 장비 아이템 자동 장착 설정
	UPROPERTY(EditDefaultsOnly, Category="Inventory|Equip")
	bool bAutoEquipOnPickup = true;
	
private:
	// 클라이언트 예측 메서드
	void PerformClientPrediction_AddItem(int ItemID, int ItemCount);
	void PerformClientPrediction_RemoveItem(int ItemID, int ItemCount);
	void PerformClientPrediction_RemoveItemByIndex(int Index);
	void PerformClientPrediction_EquipItem(int ItemID);
	void PerformClientPrediction_UnEquipItem(EEquipmentSlotType SlotType);
	void PerformClientPrediction_SwapItems(int32 FromIndex, int32 ToIndex);
	void PerformClientPrediction_SwitchWeaponSlot(int Index);
	
	UPROPERTY()
	USonheimGameInstance* m_GameInstance = nullptr;

	UPROPERTY()
	ASonheimPlayerState* m_PlayerState = nullptr;

	UPROPERTY()
	ASonheimPlayer* m_Player = nullptr;
};
