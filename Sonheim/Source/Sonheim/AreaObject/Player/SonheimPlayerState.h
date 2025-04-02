// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Net/UnrealNetwork.h"
#include "SonheimPlayerState.generated.h"

class UStatBonusComponent;
class UInventoryComponent;
class ASonheimPlayer;
class USonheimGameInstance;

// 스탯 변경 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStatsChanged, EAreaObjectStatType, StatType, float, StatValue);

/**
 * 
 */
UCLASS()
class SONHEIM_API ASonheimPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASonheimPlayerState();
	void InitPlayerState();

protected:
	virtual void BeginPlay() override;

	// 캐릭터 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	int32 Level = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	TMap<EAreaObjectStatType, float> BaseStat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	TMap<EAreaObjectStatType, float> ModifiedStat;

public:
	UPROPERTY(BlueprintReadWrite, Category="Inventory")
	UInventoryComponent* m_InventoryComponent;

	// ToDo : AreaObject로 뺄지 고민
	UPROPERTY(BlueprintReadWrite, Category="Stats")
	UStatBonusComponent* m_StatBonusComponent;

	// 스탯 업데이트 함수 추가
	UFUNCTION(BlueprintCallable, Category="Stats")
	void UpdateStats();

	UFUNCTION(BlueprintCallable, Category="Stats")
	void UpdateStat(EAreaObjectStatType StatType);

	// 스탯 값 가져오는 함수
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetStatValue(EAreaObjectStatType StatType) const;

	// 특정 스탯 설정 함수
	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetBaseStat(EAreaObjectStatType StatType, float Value);

	// 현재 적용된 스탯 값 가져오기
	UFUNCTION(BlueprintCallable, Category="Stats")
	TMap<EAreaObjectStatType, float> GetModifiedStats() const { return ModifiedStat; }

	// 스탯 변경 델리게이트
	UPROPERTY()
	FOnPlayerStatsChanged OnPlayerStatsChanged;

	ASonheimPlayer* GetSonheimPlayer();

private:
	// 무기 슬롯 변경 시 호출되는 함수 (이벤트 델리게이트에 바인딩)
	UFUNCTION()
	void OnWeaponSlotChanged(EEquipmentSlotType Slot, int ItemID);

	UPROPERTY()
	USonheimGameInstance* m_GameInstance;
	UPROPERTY()
	ASonheimPlayer* m_Player;
};
