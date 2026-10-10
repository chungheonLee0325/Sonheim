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

/** Something a player did that the server counts: the monster book and the quests listen for these. */
UENUM(BlueprintType)
enum class EPlayerDeed : uint8
{
	/** Id: the species (dt_AreaObject), defeated by the player or one of the player's pals. */
	Kill,
	/** Id: the species, captured into the player's party. */
	Capture,
	/** Id: the item, Count: how many collected from a crafting station. */
	Craft,
	/** Place: the region the player walked into. */
	Reach,
	/** Id: the dungeon's catalog number, finished with a win while the player took part. */
	ClearDungeon,
};

DECLARE_MULTICAST_DELEGATE_FourParams(FOnPlayerDeed, EPlayerDeed /*Deed*/, int32 /*Id*/, int32 /*Count*/, FName /*Place*/);

// 스탯 변경 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStatsChanged, EAreaObjectStatType, StatType, float, StatValue);

// 네트워크 동기화를 위한 스탯 구조체
USTRUCT(BlueprintType)
struct FReplicatedStat
{
	GENERATED_BODY()

	UPROPERTY()
	EAreaObjectStatType StatType;

	UPROPERTY()
	float Value;

	FReplicatedStat()
	{
		StatType = EAreaObjectStatType::None;
		Value = 0.0f;
	}

	FReplicatedStat(EAreaObjectStatType InType, float InValue)
		: StatType(InType), Value(InValue) {}
};

/**
 * 
 */
UCLASS()
class SONHEIM_API ASonheimPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASonheimPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitPlayerState();
	// 고유 ID가 정해지면 서버가 이 플레이어의 저장된 도감과 퀘스트를 읽는다.
	virtual void OnSetUniqueId() override;

	/** Server: tells the systems that count deeds (the monster book, the quests) what the player did. */
	void ReportDeed(EPlayerDeed Deed, int32 Id, int32 Count = 1, FName Place = NAME_None);
	FOnPlayerDeed OnDeed;

	/** The key the host's saves keep this player under: "Local" for the host's own player, the net id for anyone else (the same on
	 * every visit through Steam; the null subsystem of an editor play session makes a new one each session). Empty until the
	 * player's unique id is known. */
	FString GetSaveKey() const;

	UPROPERTY(BlueprintReadWrite, Category="Pal")
	class UPalInventoryComponent* m_PalInventoryComponent;
protected:
	// 캐릭터 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats", Replicated)
	int32 Level = 1;

	// 기본 스탯은 서버에서만 관리
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stats")
	TMap<EAreaObjectStatType, float> BaseStat;

	// Replicated 스탯 배열
	UPROPERTY(BlueprintReadOnly, Category="Stats", ReplicatedUsing=OnRep_ReplicatedStats)
	TArray<FReplicatedStat> ReplicatedStats;

	UFUNCTION()
	void OnRep_ReplicatedStats();

	// 로컬 캐시용
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	TMap<EAreaObjectStatType, float> ModifiedStat;

public:
	UPROPERTY(BlueprintReadWrite, Category="Inventory")
	UInventoryComponent* m_InventoryComponent;

	UPROPERTY(BlueprintReadWrite, Category="Stats")
	UStatBonusComponent* m_StatBonusComponent;

	UPROPERTY(BlueprintReadOnly, Category="MonsterDex")
	class UMonsterDexComponent* m_MonsterDexComponent;

	// 스탯 업데이트 함수 - 서버 권한
	UFUNCTION(BlueprintCallable, Category="Stats")
	void UpdateStats();

	UFUNCTION(BlueprintCallable, Category="Stats")
	void UpdateStat(EAreaObjectStatType StatType);

	// 서버 RPC
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Stats")
	void ServerUpdateStats();

	// 스탯 값 가져오는 함수
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetStatValue(EAreaObjectStatType StatType) const;

	// 특정 스탯 설정 함수 - 서버 권한
	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetBaseStat(EAreaObjectStatType StatType, float Value);

	// 현재 적용된 스탯 값 가져오기
	UFUNCTION(BlueprintCallable, Category="Stats")
	TMap<EAreaObjectStatType, float> GetModifiedStats() const { return ModifiedStat; }

	// 스탯 변경 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatsChanged OnPlayerStatsChanged;

	ASonheimPlayer* GetSonheimPlayer();

private:
	// 무기 슬롯 변경 시 호출되는 함수
	UFUNCTION()
	void OnWeaponSlotChanged(EEquipmentSlotType Slot, int ItemID);

	// Replicated 배열로 변환
	void ConvertStatsToReplicatedArray();

	UPROPERTY()
	USonheimGameInstance* m_GameInstance;
	
	UPROPERTY()
	ASonheimPlayer* m_Player;
};