// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimGameInstance.h"

#include "Sonheim/Utilities/SessionUtil.h"

namespace
{
	template <typename TObjectType>
	void AddSoftPathIfValid(const TSoftObjectPtr<TObjectType>& SoftPtr, TSet<FSoftObjectPath>& UniquePaths)
	{
		if (!SoftPtr.IsNull())
		{
			UniquePaths.Add(SoftPtr.ToSoftObjectPath());
		}
	}
}

void USonheimGameInstance::Init()
{
	Super::Init();

	FSessionUtil::Init();
	
	// AreaObject Data
	UDataTable* AreaObjectTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_AreaObject.dt_AreaObject'"));
	if (nullptr != AreaObjectTable)
	{
		TArray<FName> RowNames = AreaObjectTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FAreaObjectData* Row = AreaObjectTable->FindRow<FAreaObjectData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_AreaObject.Add(Row->AreaObjectID, *Row);
			}
		}
	}
	// Skill Data
	UDataTable* SkillTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Skill.dt_Skill'"));
	if (nullptr != SkillTable)
	{
		TArray<FName> RowNames = SkillTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FSkillData* Row = SkillTable->FindRow<FSkillData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Skill.Add(Row->SkillID, *Row);
			}
		}
	}
	// SkillBag Data
	UDataTable* SkillBagTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_SkillBag.dt_SkillBag'"));
	if (nullptr != SkillBagTable)
	{
		TArray<FName> RowNames = SkillBagTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FSkillBagData* Row = SkillBagTable->FindRow<FSkillBagData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_SkillBag.Add(Row->SkillBagID, *Row);
			}
		}
	}

	// Resource Data
	UDataTable* ResourceDataTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_ResourceObject.dt_ResourceObject'"));
	if (nullptr != ResourceDataTable)
	{
		TArray<FName> RowNames = ResourceDataTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FResourceObjectData* Row = ResourceDataTable->FindRow<FResourceObjectData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_ResourceObject.Add(Row->ResourceObjectID, *Row);
			}
		}
	}


	// Item Data
	UDataTable* ItemDataTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Item.dt_Item'"));
	if (nullptr != ItemDataTable)
	{
		TArray<FName> RowNames = ItemDataTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FItemData* Row = ItemDataTable->FindRow<FItemData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Item.Add(Row->ItemID, *Row);
			}
		}
	}

	// Level Data
	UDataTable* LevelDataTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Level.dt_Level'"));
	if (nullptr != LevelDataTable)
	{
		TArray<FName> RowNames = LevelDataTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FLevelData* Row = LevelDataTable->FindRow<FLevelData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_LevelData.Add(Row->Level, *Row);
			}
		}
	}

	// Sound Data
	UDataTable* SoundTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Sound.dt_Sound'"));
	if (nullptr != SoundTable)
	{
		TArray<FName> RowNames = SoundTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FSoundData* Row = SoundTable->FindRow<FSoundData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				SoundDataMap.Add(Row->SoundID, Row->Sound);
			}
		}
	}

	UDataTable* ContainerTable = LoadObject<UDataTable>(
	nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Container.dt_Container'"));
	if (nullptr != ContainerTable)
	{
		TArray<FName> RowNames = ContainerTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FContainerData* Row = ContainerTable->FindRow<FContainerData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Container.Add(Row->ContainerID, *Row);
			}
		}
	}

	// Quest Data
	UDataTable* QuestTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Quest.dt_Quest'"));
	if (nullptr != QuestTable)
	{
		TArray<FName> RowNames = QuestTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FQuestData* Row = QuestTable->FindRow<FQuestData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Quest.Add(Row->QuestID, *Row);
			}
		}
	}

	// Quest Reward Data (optional, merged into Reward map)
	UDataTable* QuestRewardTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_QuestReward.dt_QuestReward'"));
	if (nullptr != QuestRewardTable)
	{
		TArray<FName> RowNames = QuestRewardTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FQuestRewardRow* Row = QuestRewardTable->FindRow<FQuestRewardRow>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Reward.Add(Row->RewardID, Row->Reward);
			}
		}
	}

	// MonsterDex Data (optional)
	UDataTable* MonsterDexTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_MonsterDex.dt_MonsterDex'"));
	if (nullptr != MonsterDexTable)
	{
		TArray<FName> RowNames = MonsterDexTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FMonsterDexData* Row = MonsterDexTable->FindRow<FMonsterDexData>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_MonsterDex.Add(Row->MonsterID, *Row);
			}
		}
	}

	// MonsterDex Reward Data (optional, merged into Reward map)
	UDataTable* MonsterDexRewardTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_MonsterDexReward.dt_MonsterDexReward'"));
	if (nullptr != MonsterDexRewardTable)
	{
		TArray<FName> RowNames = MonsterDexRewardTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FMonsterDexRewardRow* Row = MonsterDexRewardTable->FindRow<FMonsterDexRewardRow>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Reward.Add(Row->RewardID, Row->Reward);
			}
		}
	}

	// Reward Data (optional, consolidated)
	UDataTable* RewardTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Reward.dt_Reward'"));
	if (nullptr != RewardTable)
	{
		TArray<FName> RowNames = RewardTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FRewardRow* Row = RewardTable->FindRow<FRewardRow>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_Reward.Add(Row->RewardID, Row->Reward);
			}
		}
	}

	// Drop Table Data (optional)
	UDataTable* DropTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_DropTable.dt_DropTable'"));
	if (nullptr != DropTable)
	{
		TArray<FName> RowNames = DropTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FDropTableRow* Row = DropTable->FindRow<FDropTableRow>(RowName, TEXT(""));
			if (nullptr != Row)
			{
				dt_DropTable.Add(Row->DropTableID, *Row);
			}
		}
	}

	StartRuntimeAssetPreload();
}

USonheimGameInstance* USonheimGameInstance::Get(class UWorld* World)
{
	return Cast<USonheimGameInstance>(World->GetGameInstance());
}

FAreaObjectData* USonheimGameInstance::GetDataAreaObject(const int AreaObjectID)
{
	if (FAreaObjectData* data = dt_AreaObject.Find(AreaObjectID))
	{
		return data;
	}

	return nullptr;
}

FSkillData* USonheimGameInstance::GetDataSkill(int SkillID)
{
	if (FSkillData* data = dt_Skill.Find(SkillID))
	{
		return data;
	}

	return nullptr;
}

FSkillBagData* USonheimGameInstance::GetDataSkillBag(int SkillBagID)
{
	if (FSkillBagData* data = dt_SkillBag.Find(SkillBagID))
	{
		return data;
	}

	return nullptr;
}

FResourceObjectData* USonheimGameInstance::GetDataResourceObject(int ResourceObjectID)
{
	if (FResourceObjectData* data = dt_ResourceObject.Find(ResourceObjectID))
	{
		return data;
	}

	return nullptr;
}

FItemData* USonheimGameInstance::GetDataItem(int ItemID)
{
	if (FItemData* data = dt_Item.Find(ItemID))
	{
		return data;
	}

	return nullptr;
}

TMap<int32, FLevelData>* USonheimGameInstance::GetDataLevel()
{
	if (!dt_LevelData.IsEmpty())
	{
		return &dt_LevelData;
	}

	return nullptr;
}

FContainerData* USonheimGameInstance::GetDataContainer(int ContainerID)
{
	if (FContainerData* data = dt_Container.Find(ContainerID))
	{
		return data;
	}
	return nullptr;
}

FQuestData* USonheimGameInstance::GetDataQuest(int QuestID)
{
	if (FQuestData* data = dt_Quest.Find(QuestID))
	{
		return data;
	}
	return nullptr;
}

const FRewardDef* USonheimGameInstance::GetRewardDef(int RewardID) const
{
	if (const FRewardDef* data = dt_Reward.Find(RewardID))
	{
		return data;
	}
	return nullptr;
}

const FRewardDef* USonheimGameInstance::GetDataQuestReward(int RewardID) const
{
	return GetRewardDef(RewardID);
}

FMonsterDexData* USonheimGameInstance::GetDataMonsterDex(int MonsterID)
{
	if (FMonsterDexData* data = dt_MonsterDex.Find(MonsterID))
	{
		return data;
	}
	return nullptr;
}

const FRewardDef* USonheimGameInstance::GetDataMonsterDexReward(int RewardID) const
{
	return GetRewardDef(RewardID);
}

const FDropTableRow* USonheimGameInstance::GetDropTable(int DropTableID) const
{
	if (const FDropTableRow* Row = dt_DropTable.Find(DropTableID))
	{
		return Row;
	}
	return nullptr;
}

void USonheimGameInstance::SaveSessionQuestSnapshot(const FString& PlayerKey, const FQuestPlayerSnapshot& Snapshot)
{
	if (PlayerKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip saving quest snapshot: empty player key."));
		return;
	}
	SessionQuestSnapshots.Add(PlayerKey, Snapshot);
}

bool USonheimGameInstance::LoadSessionQuestSnapshot(const FString& PlayerKey, FQuestPlayerSnapshot& OutSnapshot) const
{
	if (PlayerKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip loading quest snapshot: empty player key."));
		return false;
	}

	if (const FQuestPlayerSnapshot* Found = SessionQuestSnapshots.Find(PlayerKey))
	{
		OutSnapshot = *Found;
		return true;
	}
	return false;
}

void USonheimGameInstance::ClearSessionQuestSnapshot(const FString& PlayerKey)
{
	if (PlayerKey.IsEmpty())
	{
		return;
	}
	SessionQuestSnapshots.Remove(PlayerKey);
}

void USonheimGameInstance::CollectRuntimePreloadAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	TSet<FSoftObjectPath> UniquePaths;

	for (const TPair<int32, FAreaObjectData>& Pair : dt_AreaObject)
	{
		const FAreaObjectData& Data = Pair.Value;
		AddSoftPathIfValid(Data.Die_AnimMontage, UniquePaths);
		AddSoftPathIfValid(Data.Stagger_AnimMontage, UniquePaths);
		AddSoftPathIfValid(Data.AreaObjectIcon, UniquePaths);
	}

	for (const TPair<int32, FSkillData>& Pair : dt_Skill)
	{
		const FSkillData& Skill = Pair.Value;
		AddSoftPathIfValid(Skill.Montage, UniquePaths);
		for (const FAttackData& Attack : Skill.AttackData)
		{
			AddSoftPathIfValid(Attack.FireSFX, UniquePaths);
			AddSoftPathIfValid(Attack.FireVFX_N, UniquePaths);
			AddSoftPathIfValid(Attack.FireVFX2_N, UniquePaths);
			AddSoftPathIfValid(Attack.HitSFX, UniquePaths);
			AddSoftPathIfValid(Attack.HitVFX_N, UniquePaths);
			AddSoftPathIfValid(Attack.HitVFX_P, UniquePaths);
		}
	}

	for (const TPair<int32, FResourceObjectData>& Pair : dt_ResourceObject)
	{
		const FResourceObjectData& Data = Pair.Value;
		AddSoftPathIfValid(Data.ResourceMesh, UniquePaths);
		AddSoftPathIfValid(Data.HarvestEffect, UniquePaths);
		AddSoftPathIfValid(Data.DestroyEffect, UniquePaths);
	}

	for (const TPair<int32, FItemData>& Pair : dt_Item)
	{
		const FItemData& Data = Pair.Value;
		AddSoftPathIfValid(Data.ItemIcon, UniquePaths);
		AddSoftPathIfValid(Data.ItemMesh, UniquePaths);
		AddSoftPathIfValid(Data.EquipmentData.EquipmentMesh, UniquePaths);
		AddSoftPathIfValid(Data.EquipmentData.EquipmentAnim, UniquePaths);
	}

	for (const TPair<int32, FContainerData>& Pair : dt_Container)
	{
		const FContainerData& Data = Pair.Value;
		AddSoftPathIfValid(Data.ContainerMesh, UniquePaths);
		AddSoftPathIfValid(Data.OpenSound, UniquePaths);
		AddSoftPathIfValid(Data.CloseSound, UniquePaths);
	}

	for (const TPair<int32, FMonsterDexData>& Pair : dt_MonsterDex)
	{
		AddSoftPathIfValid(Pair.Value.Icon, UniquePaths);
	}

	for (const TPair<int, TSoftObjectPtr<USoundBase>>& Pair : SoundDataMap)
	{
		AddSoftPathIfValid(Pair.Value, UniquePaths);
	}

	OutPaths.Reset();
	OutPaths.Reserve(UniquePaths.Num());
	for (const FSoftObjectPath& Path : UniquePaths)
	{
		OutPaths.Add(Path);
	}
}

void USonheimGameInstance::OnRuntimeAssetPreloadComplete()
{
	bRuntimeAssetPreloadComplete = true;
}

void USonheimGameInstance::StartRuntimeAssetPreload()
{
	if (RuntimeAssetPreloadHandle.IsValid() && !RuntimeAssetPreloadHandle->HasLoadCompleted())
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	CollectRuntimePreloadAssetPaths(Paths);
	if (Paths.IsEmpty())
	{
		bRuntimeAssetPreloadComplete = true;
		RuntimeAssetPreloadHandle.Reset();
		return;
	}

	bRuntimeAssetPreloadComplete = false;
	RuntimeAssetPreloadHandle = RuntimeAssetStreamableManager.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &USonheimGameInstance::OnRuntimeAssetPreloadComplete),
		FStreamableManager::AsyncLoadHighPriority);

	if (!RuntimeAssetPreloadHandle.IsValid())
	{
		bRuntimeAssetPreloadComplete = true;
	}
}
