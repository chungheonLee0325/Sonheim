// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/Quest/QuestData.h"
#include "Sonheim/Quest/QuestSnapshot.h"
#include "Sonheim/MonsterDex/MonsterDexData.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "SonheimGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USonheimGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	static USonheimGameInstance* Get(class UWorld* World);
	
	FAreaObjectData* GetDataAreaObject(int AreaObjectID);
	FSkillData* GetDataSkill(int SkillID);
	FSkillBagData* GetDataSkillBag(int SkillBagID);
	FResourceObjectData* GetDataResourceObject(int ResourceObjectID);
	FItemData* GetDataItem(int ItemID);
	TMap<int32, FLevelData>* GetDataLevel();
	FContainerData* GetDataContainer(int ContainerID);
	FQuestData* GetDataQuest(int QuestID);
	const FRewardDef* GetRewardDef(int RewardID) const;
	const FRewardDef* GetDataQuestReward(int RewardID) const;
	FMonsterDexData* GetDataMonsterDex(int MonsterID);
	const FRewardDef* GetDataMonsterDexReward(int RewardID) const;
	const struct FDropTableRow* GetDropTable(int DropTableID) const;
	const TMap<int32, FMonsterDexData>& GetMonsterDexDataMap() const { return dt_MonsterDex; }
	void StartRuntimeAssetPreload();
	bool IsRuntimeAssetPreloadComplete() const { return bRuntimeAssetPreloadComplete; }

	// Session-only quest snapshot cache (server travel support)
	void SaveSessionQuestSnapshot(const FString& PlayerKey, const FQuestPlayerSnapshot& Snapshot);
	bool LoadSessionQuestSnapshot(const FString& PlayerKey, FQuestPlayerSnapshot& OutSnapshot) const;
	void ClearSessionQuestSnapshot(const FString& PlayerKey);
    
	UPROPERTY()
	TMap<int32, FAreaObjectData> dt_AreaObject;
	UPROPERTY()
	TMap<int32, FSkillData> dt_Skill;
	UPROPERTY()
	TMap<int32, FSkillBagData> dt_SkillBag;
	UPROPERTY()
	TMap<int32, FResourceObjectData> dt_ResourceObject;
	UPROPERTY()
	TMap<int32, FItemData> dt_Item;
	UPROPERTY()
	TMap<int32, FLevelData> dt_LevelData;
	UPROPERTY()
	TMap<int32, FContainerData> dt_Container;
	UPROPERTY()
	TMap<int32, FQuestData> dt_Quest;
	UPROPERTY()
	TMap<int32, FRewardDef> dt_Reward;
	UPROPERTY()
	TMap<int32, FMonsterDexData> dt_MonsterDex;
	UPROPERTY()
	TMap<int32, FDropTableRow> dt_DropTable;

	UPROPERTY()
	TMap<FString, FQuestPlayerSnapshot> SessionQuestSnapshots;

	UPROPERTY(EditAnywhere)
	TMap<int, TSoftObjectPtr<USoundBase>> SoundDataMap;

	UPROPERTY(EditAnywhere)
	uint8 MaxPlayer{};
	UPROPERTY(EditAnywhere)
	FString RoomName{};
protected:
	virtual void Init() override;

private:
	void CollectRuntimePreloadAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;
	void OnRuntimeAssetPreloadComplete();

private:
	FStreamableManager RuntimeAssetStreamableManager;
	TSharedPtr<FStreamableHandle> RuntimeAssetPreloadHandle;
	bool bRuntimeAssetPreloadComplete = false;

};
