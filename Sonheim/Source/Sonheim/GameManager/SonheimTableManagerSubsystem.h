#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"

#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/ResourceManager/SonheimVariantData.h"
#include "Sonheim/Quest/QuestData.h"
#include "Sonheim/MonsterDex/MonsterDexData.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "Sonheim/UI/System/UIStackData.h"

#include "SonheimTableManagerSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnSonheimRuntimeDataReady);

UCLASS()
class SONHEIM_API USonheimTableManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsReady() const { return bIsReady; }
	bool HasFailed() const { return bHasFailed; }
	int32 GetPreReadyAccessCount() const { return PreReadyAccessCount; }
	FOnSonheimRuntimeDataReady& OnReady() { return OnRuntimeDataReady; }

	const FAreaObjectData* FindAreaObject(int32 Id) const;
	const FSkillData* FindSkill(int32 Id) const;
	const FSkillBagData* FindSkillBag(int32 Id) const;
	const FResourceObjectData* FindResourceObject(int32 Id) const;
	const FItemData* FindItem(int32 Id) const;
	const FLevelData* FindLevel(int32 Level) const;
	const FContainerData* FindContainer(int32 Id) const;
	const FQuestData* FindQuest(int32 Id) const;
	const FRewardDef* FindReward(int32 Id) const;
	const FMonsterDexData* FindMonsterDex(int32 Id) const;
	const FDropTableRow* FindDropTable(int32 Id) const;
	const FUIWidgetDefRow* FindUIWidgetDef(FName UIId) const;
	const FUIWidgetPresetRow* FindUIPreset(FName PresetId) const;
	const TSoftObjectPtr<USoundBase>* FindSound(int32 SoundID) const;

	const TMap<int32, FMonsterDexData>& GetMonsterDexDataMap() const { return MonsterDexDataMap; }
	const TMap<int32, TSoftObjectPtr<USoundBase>>& GetSoundDataMap() const { return SoundDataMap; }

private:
	void RequestDataTablesAsyncLoad();
	void OnDataTablesLoaded();
	void RequestCoreVariantsAsyncLoad();
	void OnCoreVariantsLoaded();
	void OnSelectedAssetPreloadComplete();
	void BuildSelectedAssetPreload();
	void MarkPreReadyAccess(const TCHAR* Context) const;
	bool IsRuntimeDataAccessible(const TCHAR* Context) const;
	void ResetRuntimeData();

private:
	TSoftObjectPtr<UDataTable> DT_AreaObject;
	TSoftObjectPtr<UDataTable> DT_Skill;
	TSoftObjectPtr<UDataTable> DT_SkillBag;
	TSoftObjectPtr<UDataTable> DT_ResourceObject;
	TSoftObjectPtr<UDataTable> DT_Item;
	TSoftObjectPtr<UDataTable> DT_Level;
	TSoftObjectPtr<UDataTable> DT_Sound;
	TSoftObjectPtr<UDataTable> DT_Container;
	TSoftObjectPtr<UDataTable> DT_Quest;
	TSoftObjectPtr<UDataTable> DT_QuestReward;
	TSoftObjectPtr<UDataTable> DT_MonsterDex;
	TSoftObjectPtr<UDataTable> DT_MonsterDexReward;
	TSoftObjectPtr<UDataTable> DT_Reward;
	TSoftObjectPtr<UDataTable> DT_DropTable;
	TSoftObjectPtr<UDataTable> DT_UIWidgetDef;
	TSoftObjectPtr<UDataTable> DT_UIPreset;

	TMap<int32, FAreaObjectData> AreaObjectDataMap;
	TMap<int32, FSkillData> SkillDataMap;
	TMap<int32, FSkillBagData> SkillBagDataMap;
	TMap<int32, FResourceObjectData> ResourceObjectDataMap;
	TMap<int32, FItemData> ItemDataMap;
	TMap<int32, FLevelData> LevelDataMap;
	TMap<int32, FContainerData> ContainerDataMap;
	TMap<int32, FQuestData> QuestDataMap;
	TMap<int32, FRewardDef> RewardDataMap;
	TMap<int32, FMonsterDexData> MonsterDexDataMap;
	TMap<int32, FDropTableRow> DropTableDataMap;
	TMap<int32, TSoftObjectPtr<USoundBase>> SoundDataMap;
	TMap<FName, FUIWidgetDefRow> UIWidgetDefMap;
	TMap<FName, FUIWidgetPresetRow> UIPresetMap;

	TMap<int32, FSoftObjectPath> AreaObjectVariantPathMap;
	TMap<int32, FSoftObjectPath> ItemVariantPathMap;
	TMap<int32, FSoftObjectPath> ResourceObjectVariantPathMap;
	TMap<int32, FSoftObjectPath> ContainerVariantPathMap;
	TMap<int32, FSoftObjectPath> MonsterDexVariantPathMap;

	FStreamableManager RuntimeDataStreamableManager;
	TSharedPtr<FStreamableHandle> RuntimeDataLoadHandle;
	TSharedPtr<FStreamableHandle> CoreVariantLoadHandle;
	TSharedPtr<FStreamableHandle> SelectedAssetPreloadHandle;
	FOnSonheimRuntimeDataReady OnRuntimeDataReady;

	bool bIsReady = false;
	bool bHasFailed = false;
	bool bSelectedAssetPreloadComplete = false;

	mutable int32 PreReadyAccessCount = 0;
	mutable TSet<FName> PreReadyWarningContexts;
};
