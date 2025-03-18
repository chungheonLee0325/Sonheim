// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const TArray<FInventoryItem>&, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlotType, Slot, int, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, int, ItemID, int, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, int, ItemID, int, Count);

class USonheimGameInstance;
class ASonheimPlayer;

// ToDo : 아이템 스왑, 무기 장착 빈공간 + 1번스왑, 장비 장착시 제거
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// 인벤토리 관련 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	int32 MaxInventorySlots = 50;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<FInventoryItem> InventoryItems;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TMap<EEquipmentSlotType, int> EquippedItems;
	
	// 현재 선택된 무기 슬롯 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Inventory")
	EEquipmentSlotType CurrentWeaponSlot = EEquipmentSlotType::Weapon1;
	
	// 인벤토리 관련 함수
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItem(int ItemID, int ItemCount);
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveItem(int ItemID, int ItemCount);
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItem(int ItemID);
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool UnequipItem(EEquipmentSlotType SlotType);
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool HasItem(int ItemID, int RequiredCount = 1) const;
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	int GetItemCount(int ItemID) const;
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	TArray<FInventoryItem> GetInventory() const;
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	TMap<EEquipmentSlotType, int> GetEquippedItems() const;

	// 인벤토리 인덱스 기반 장착 함수
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItemByIndex(int32 InventoryIndex);
    
	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 GetEquippedItemIndex(EEquipmentSlotType SlotType) const;

	// 무기 전용 장착 함수
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipWeaponToSlot(int32 InventoryIndex, EEquipmentSlotType WeaponSlot);
	
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SwitchWeaponSlot(EEquipmentSlotType NewWeaponSlot);
	
	// 이벤트 델리게이트
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnInventoryChanged OnInventoryChanged;
    
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnEquipmentChanged OnEquipmentChanged;
    
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnItemAdded OnItemAdded;
    
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnItemRemoved OnItemRemoved;
	
private:
	UPROPERTY()
	USonheimGameInstance* m_GameInstance;
	
	UPROPERTY()
	ASonheimPlayer* m_Player;
    
	// 내부 헬퍼 함수
	bool IsValidItemID(int ItemID) const;
	struct FItemData* GetItemData(int ItemID) const;
	void ApplyEquipmentStats(int ItemID, bool bEquipping);
	int FindItemIndexInInventory(int ItemID) const;
	void BroadcastInventoryChanged();
};
