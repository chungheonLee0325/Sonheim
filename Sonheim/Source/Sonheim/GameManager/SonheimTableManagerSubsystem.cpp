#include "SonheimTableManagerSubsystem.h"

#include "Engine/AssetManager.h"

#include "Sonheim/Quest/QuestData.h"
#include "Sonheim/MonsterDex/MonsterDexData.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "Sonheim/Utilities/LogMacro.h"

namespace
{
	static const FPrimaryAssetType AreaObjectVariantAssetType(TEXT("AreaObjectVariant"));
	static const FPrimaryAssetType ItemVariantAssetType(TEXT("ItemVariant"));
	static const FPrimaryAssetType ResourceObjectVariantAssetType(TEXT("ResourceObjectVariant"));
	static const FPrimaryAssetType ContainerVariantAssetType(TEXT("ContainerVariant"));
	static const FPrimaryAssetType MonsterDexVariantAssetType(TEXT("MonsterDexVariant"));
	static const FPrimaryAssetType UIWidgetDefVariantAssetType(TEXT("UIWidgetDefVariant"));
	static const FPrimaryAssetType UIPresetVariantAssetType(TEXT("UIPresetVariant"));

	template <typename TRowType>
	void CopyRowsToMapByIntKey(UDataTable* Table, TMap<int32, TRowType>& OutMap, TFunctionRef<int32(const TRowType&)> KeySelector)
	{
		check(Table);
		TArray<FName> RowNames = Table->GetRowNames();
		OutMap.Reserve(RowNames.Num());
		for (const FName& RowName : RowNames)
		{
			const TRowType* Row = Table->FindRow<TRowType>(RowName, TEXT("TableManager.Load"));
			checkf(Row, TEXT("[TableManager] Failed to read row '%s' in table '%s'."), *RowName.ToString(), *Table->GetName());
			const int32 Key = KeySelector(*Row);
			checkf(!OutMap.Contains(Key),
				TEXT("[TableManager] Duplicate key detected while loading table '%s'. Key=%d Row=%s"),
				*Table->GetName(),
				Key,
				*RowName.ToString());
			OutMap.Add(Key, *Row);
		}
	}

	template <typename TObjectType>
	void AddSoftPathIfValid(const TSoftObjectPtr<TObjectType>& SoftPtr, TSet<FSoftObjectPath>& UniquePaths)
	{
		if (!SoftPtr.IsNull())
		{
			UniquePaths.Add(SoftPtr.ToSoftObjectPath());
		}
	}

	template <typename TObjectType>
	void AddSoftClassPathIfValid(const TSoftClassPtr<TObjectType>& SoftPtr, TSet<FSoftObjectPath>& UniquePaths)
	{
		if (!SoftPtr.IsNull())
		{
			UniquePaths.Add(SoftPtr.ToSoftObjectPath());
		}
	}

	template <typename TRowType>
	void CollectVariantAssetPaths(
		const UAssetManager& AssetManager,
		const TCHAR* DomainName,
		const FPrimaryAssetType& AssetType,
		const TMap<int32, TRowType>& SourceMap,
		TFunctionRef<FName(const TRowType&)> VariantIdSelector,
		TMap<int32, FSoftObjectPath>& OutVariantPathMap,
		TSet<FSoftObjectPath>& OutUniqueVariantPaths)
	{
		TSet<FName> UniqueVariantIds;
		OutVariantPathMap.Reserve(SourceMap.Num());

		for (const TPair<int32, TRowType>& Pair : SourceMap)
		{
			const int32 DomainId = Pair.Key;
			const FName VariantId = VariantIdSelector(Pair.Value);
			checkf(!VariantId.IsNone(),
				TEXT("[TableManager] Missing VariantId in %s row. DomainId=%d"),
				DomainName,
				DomainId);

			checkf(!UniqueVariantIds.Contains(VariantId),
				TEXT("[TableManager] Duplicate VariantId in %s rows. VariantId=%s"),
				DomainName,
				*VariantId.ToString());
			UniqueVariantIds.Add(VariantId);

			const FPrimaryAssetId PrimaryAssetId(AssetType, VariantId);
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
			checkf(AssetPath.IsValid(),
				TEXT("[TableManager] Failed to resolve primary asset path. Domain=%s DomainId=%d VariantId=%s PrimaryAssetType=%s"),
				DomainName,
				DomainId,
				*VariantId.ToString(),
				*AssetType.ToString());

			OutVariantPathMap.Add(DomainId, AssetPath);
			OutUniqueVariantPaths.Add(AssetPath);
		}
	}

	template <typename TVariantType>
	TVariantType* ResolveVariantAsset(const FSoftObjectPath& AssetPath, const TCHAR* DomainName, int32 DomainId)
	{
		UObject* LoadedObject = AssetPath.ResolveObject();
		if (!LoadedObject)
		{
			LoadedObject = AssetPath.TryLoad();
		}

		TVariantType* Variant = Cast<TVariantType>(LoadedObject);
		checkf(Variant,
			TEXT("[TableManager] Failed to load variant asset for %s row. DomainId=%d AssetPath=%s ExpectedClass=%s"),
			DomainName,
			DomainId,
			*AssetPath.ToString(),
			*TVariantType::StaticClass()->GetName());
		return Variant;
	}

	template <typename TVariantType>
	TVariantType* ResolveVariantAsset(const FSoftObjectPath& AssetPath, const TCHAR* DomainName, FName DomainKey)
	{
		UObject* LoadedObject = AssetPath.ResolveObject();
		if (!LoadedObject)
		{
			LoadedObject = AssetPath.TryLoad();
		}

		TVariantType* Variant = Cast<TVariantType>(LoadedObject);
		checkf(Variant,
			TEXT("[TableManager] Failed to load variant asset for %s row. DomainKey=%s AssetPath=%s ExpectedClass=%s"),
			DomainName,
			*DomainKey.ToString(),
			*AssetPath.ToString(),
			*TVariantType::StaticClass()->GetName());
		return Variant;
	}

	template <typename TRowType>
	void CollectVariantAssetPathsByName(
		const UAssetManager& AssetManager,
		const TCHAR* DomainName,
		const FPrimaryAssetType& AssetType,
		const TMap<FName, TRowType>& SourceMap,
		TFunctionRef<FName(const TRowType&)> VariantIdSelector,
		TMap<FName, FSoftObjectPath>& OutVariantPathMap,
		TSet<FSoftObjectPath>& OutUniqueVariantPaths)
	{
		TSet<FName> UniqueVariantIds;
		OutVariantPathMap.Reserve(SourceMap.Num());

		for (const TPair<FName, TRowType>& Pair : SourceMap)
		{
			const FName DomainKey = Pair.Key;
			const FName VariantId = VariantIdSelector(Pair.Value);
			checkf(!VariantId.IsNone(),
				TEXT("[TableManager] Missing VariantId in %s row. DomainKey=%s"),
				DomainName,
				*DomainKey.ToString());

			checkf(!UniqueVariantIds.Contains(VariantId),
				TEXT("[TableManager] Duplicate VariantId in %s rows. VariantId=%s"),
				DomainName,
				*VariantId.ToString());
			UniqueVariantIds.Add(VariantId);

			const FPrimaryAssetId PrimaryAssetId(AssetType, VariantId);
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
			checkf(AssetPath.IsValid(),
				TEXT("[TableManager] Failed to resolve primary asset path. Domain=%s DomainKey=%s VariantId=%s PrimaryAssetType=%s"),
				DomainName,
				*DomainKey.ToString(),
				*VariantId.ToString(),
				*AssetType.ToString());

			OutVariantPathMap.Add(DomainKey, AssetPath);
			OutUniqueVariantPaths.Add(AssetPath);
		}
	}
}

void USonheimTableManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DT_AreaObject = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_AreaObject.dt_AreaObject'")));
	DT_Skill = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Skill.dt_Skill'")));
	DT_SkillBag = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_SkillBag.dt_SkillBag'")));
	DT_ResourceObject = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_ResourceObject.dt_ResourceObject'")));
	DT_Item = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Item.dt_Item'")));
	DT_Level = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Level.dt_Level'")));
	DT_Sound = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Sound.dt_Sound'")));
	DT_Container = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Container.dt_Container'")));
	DT_Quest = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Quest.dt_Quest'")));
	DT_QuestReward = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_QuestReward.dt_QuestReward'")));
	DT_MonsterDex = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_MonsterDex.dt_MonsterDex'")));
	DT_MonsterDexReward = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_MonsterDexReward.dt_MonsterDexReward'")));
	DT_Reward = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_Reward.dt_Reward'")));
	DT_DropTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_DropTable.dt_DropTable'")));
	DT_UIWidgetDef = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_UIWidgetDef.dt_UIWidgetDef'")));
	DT_UIPreset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_UIPreset.dt_UIPreset'")));

	RequestDataTablesAsyncLoad();
}

void USonheimTableManagerSubsystem::Deinitialize()
{
	RuntimeDataLoadHandle.Reset();
	CoreVariantLoadHandle.Reset();
	UIVariantLoadHandle.Reset();
	SelectedAssetPreloadHandle.Reset();
	OnRuntimeDataReady.Clear();
	ResetRuntimeData();

	Super::Deinitialize();
}

void USonheimTableManagerSubsystem::ResetRuntimeData()
{
	bIsReady = false;
	bHasFailed = false;
	bSelectedAssetPreloadComplete = false;
	PreReadyAccessCount = 0;
	PreReadyWarningContexts.Reset();

	AreaObjectDataMap.Reset();
	SkillDataMap.Reset();
	SkillBagDataMap.Reset();
	ResourceObjectDataMap.Reset();
	ItemDataMap.Reset();
	LevelDataMap.Reset();
	ContainerDataMap.Reset();
	QuestDataMap.Reset();
	RewardDataMap.Reset();
	MonsterDexDataMap.Reset();
	DropTableDataMap.Reset();
	SoundDataMap.Reset();
	UIWidgetDefMap.Reset();
	UIPresetMap.Reset();

	AreaObjectVariantPathMap.Reset();
	ItemVariantPathMap.Reset();
	ResourceObjectVariantPathMap.Reset();
	ContainerVariantPathMap.Reset();
	MonsterDexVariantPathMap.Reset();
	UIWidgetDefVariantPathMap.Reset();
	UIPresetVariantPathMap.Reset();

	UIWidgetDefVariantMap.Reset();
	UIPresetVariantMap.Reset();
}

void USonheimTableManagerSubsystem::RequestDataTablesAsyncLoad()
{
	ResetRuntimeData();

	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(16);

	auto AddPath = [&Paths](const TSoftObjectPtr<UDataTable>& TablePtr)
	{
		if (!TablePtr.IsNull())
		{
			Paths.Add(TablePtr.ToSoftObjectPath());
		}
	};

	AddPath(DT_AreaObject);
	AddPath(DT_Skill);
	AddPath(DT_SkillBag);
	AddPath(DT_ResourceObject);
	AddPath(DT_Item);
	AddPath(DT_Level);
	AddPath(DT_Sound);
	AddPath(DT_Container);
	AddPath(DT_Quest);
	AddPath(DT_QuestReward);
	AddPath(DT_MonsterDex);
	AddPath(DT_MonsterDexReward);
	AddPath(DT_Reward);
	AddPath(DT_DropTable);
	AddPath(DT_UIWidgetDef);
	AddPath(DT_UIPreset);

	if (Paths.IsEmpty())
	{
		bHasFailed = true;
		ensureAlwaysMsgf(false, TEXT("[TableManager] DataTable path registry is empty."));
		checkf(false, TEXT("[TableManager] DataTable path registry is empty."));
		return;
	}

	RuntimeDataLoadHandle = RuntimeDataStreamableManager.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &USonheimTableManagerSubsystem::OnDataTablesLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!RuntimeDataLoadHandle.IsValid())
	{
		bHasFailed = true;
		ensureAlwaysMsgf(false, TEXT("[TableManager] Failed to request async DataTable load handle."));
		checkf(false, TEXT("[TableManager] Failed to request async DataTable load handle."));
	}
}

void USonheimTableManagerSubsystem::OnDataTablesLoaded()
{
	UDataTable* AreaObjectTable = DT_AreaObject.Get();
	UDataTable* SkillTable = DT_Skill.Get();
	UDataTable* SkillBagTable = DT_SkillBag.Get();
	UDataTable* ResourceTable = DT_ResourceObject.Get();
	UDataTable* ItemTable = DT_Item.Get();
	UDataTable* LevelTable = DT_Level.Get();
	UDataTable* SoundTable = DT_Sound.Get();
	UDataTable* ContainerTable = DT_Container.Get();
	UDataTable* QuestTable = DT_Quest.Get();
	UDataTable* QuestRewardTable = DT_QuestReward.Get();
	UDataTable* MonsterDexTable = DT_MonsterDex.Get();
	UDataTable* MonsterDexRewardTable = DT_MonsterDexReward.Get();
	UDataTable* RewardTable = DT_Reward.Get();
	UDataTable* DropTable = DT_DropTable.Get();
	UDataTable* UIWidgetDefTable = DT_UIWidgetDef.Get();
	UDataTable* UIPresetTable = DT_UIPreset.Get();

	auto RequireTable = [this](UDataTable* Table, const TCHAR* TableName) -> bool
	{
		if (Table)
		{
			return true;
		}

		bHasFailed = true;
		bIsReady = false;
		ensureAlwaysMsgf(false, TEXT("[TableManager] Missing required table: %s"), TableName);
		checkf(false, TEXT("[TableManager] Missing required table: %s"), TableName);
		return false;
	};

	if (!RequireTable(AreaObjectTable, TEXT("dt_AreaObject"))) return;
	if (!RequireTable(SkillTable, TEXT("dt_Skill"))) return;
	if (!RequireTable(SkillBagTable, TEXT("dt_SkillBag"))) return;
	if (!RequireTable(ResourceTable, TEXT("dt_ResourceObject"))) return;
	if (!RequireTable(ItemTable, TEXT("dt_Item"))) return;
	if (!RequireTable(LevelTable, TEXT("dt_Level"))) return;
	if (!RequireTable(SoundTable, TEXT("dt_Sound"))) return;
	if (!RequireTable(ContainerTable, TEXT("dt_Container"))) return;
	if (!RequireTable(QuestTable, TEXT("dt_Quest"))) return;
	if (!RequireTable(QuestRewardTable, TEXT("dt_QuestReward"))) return;
	if (!RequireTable(MonsterDexTable, TEXT("dt_MonsterDex"))) return;
	if (!RequireTable(MonsterDexRewardTable, TEXT("dt_MonsterDexReward"))) return;
	if (!RequireTable(RewardTable, TEXT("dt_Reward"))) return;
	if (!RequireTable(DropTable, TEXT("dt_DropTable"))) return;
	if (!RequireTable(UIWidgetDefTable, TEXT("dt_UIWidgetDef"))) return;
	if (!RequireTable(UIPresetTable, TEXT("dt_UIPreset"))) return;

	CopyRowsToMapByIntKey<FAreaObjectData>(AreaObjectTable, AreaObjectDataMap, [](const FAreaObjectData& Row) { return Row.AreaObjectID; });
	CopyRowsToMapByIntKey<FSkillData>(SkillTable, SkillDataMap, [](const FSkillData& Row) { return Row.SkillID; });
	CopyRowsToMapByIntKey<FSkillBagData>(SkillBagTable, SkillBagDataMap, [](const FSkillBagData& Row) { return Row.SkillBagID; });
	CopyRowsToMapByIntKey<FResourceObjectData>(ResourceTable, ResourceObjectDataMap, [](const FResourceObjectData& Row) { return Row.ResourceObjectID; });
	CopyRowsToMapByIntKey<FItemData>(ItemTable, ItemDataMap, [](const FItemData& Row) { return Row.ItemID; });
	CopyRowsToMapByIntKey<FLevelData>(LevelTable, LevelDataMap, [](const FLevelData& Row) { return Row.Level; });
	CopyRowsToMapByIntKey<FContainerData>(ContainerTable, ContainerDataMap, [](const FContainerData& Row) { return Row.ContainerID; });
	CopyRowsToMapByIntKey<FQuestData>(QuestTable, QuestDataMap, [](const FQuestData& Row) { return Row.QuestID; });
	CopyRowsToMapByIntKey<FMonsterDexData>(MonsterDexTable, MonsterDexDataMap, [](const FMonsterDexData& Row) { return Row.MonsterID; });
	CopyRowsToMapByIntKey<FDropTableRow>(DropTable, DropTableDataMap, [](const FDropTableRow& Row) { return Row.DropTableID; });

	SoundDataMap.Reset();
	{
		TArray<FName> RowNames = SoundTable->GetRowNames();
		SoundDataMap.Reserve(RowNames.Num());
		for (const FName& RowName : RowNames)
		{
			const FSoundData* Row = SoundTable->FindRow<FSoundData>(RowName, TEXT("TableManager.LoadSound"));
			checkf(Row, TEXT("[TableManager] Failed to read sound row '%s'."), *RowName.ToString());
			SoundDataMap.Add(Row->SoundID, Row->Sound);
		}
	}

	RewardDataMap.Reset();
	{
		TArray<FName> RowNames = QuestRewardTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			const FQuestRewardRow* Row = QuestRewardTable->FindRow<FQuestRewardRow>(RowName, TEXT("TableManager.LoadQuestReward"));
			checkf(Row, TEXT("[TableManager] Failed to read quest reward row '%s'."), *RowName.ToString());
			RewardDataMap.Add(Row->RewardID, Row->Reward);
		}
	}
	{
		TArray<FName> RowNames = MonsterDexRewardTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			const FMonsterDexRewardRow* Row = MonsterDexRewardTable->FindRow<FMonsterDexRewardRow>(RowName, TEXT("TableManager.LoadDexReward"));
			checkf(Row, TEXT("[TableManager] Failed to read monster dex reward row '%s'."), *RowName.ToString());
			RewardDataMap.Add(Row->RewardID, Row->Reward);
		}
	}
	{
		TArray<FName> RowNames = RewardTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			const FRewardRow* Row = RewardTable->FindRow<FRewardRow>(RowName, TEXT("TableManager.LoadReward"));
			checkf(Row, TEXT("[TableManager] Failed to read reward row '%s'."), *RowName.ToString());
			RewardDataMap.Add(Row->RewardID, Row->Reward);
		}
	}

	UIWidgetDefMap.Reset();
	{
		TArray<FName> RowNames = UIWidgetDefTable->GetRowNames();
		UIWidgetDefMap.Reserve(RowNames.Num());
		for (const FName& RowName : RowNames)
		{
			const FUIWidgetDefRow* Row = UIWidgetDefTable->FindRow<FUIWidgetDefRow>(RowName, TEXT("TableManager.LoadUIWidgetDef"));
			checkf(Row, TEXT("[TableManager] Failed to read UI widget def row '%s'."), *RowName.ToString());
			UIWidgetDefMap.Add(Row->UIId, *Row);
		}
	}

	UIPresetMap.Reset();
	{
		TArray<FName> RowNames = UIPresetTable->GetRowNames();
		UIPresetMap.Reserve(RowNames.Num());
		for (const FName& RowName : RowNames)
		{
			const FUIWidgetPresetRow* Row = UIPresetTable->FindRow<FUIWidgetPresetRow>(RowName, TEXT("TableManager.LoadUIPreset"));
			checkf(Row, TEXT("[TableManager] Failed to read UI preset row '%s'."), *RowName.ToString());
			UIPresetMap.Add(Row->PresetId, *Row);
		}
	}

	RequestCoreVariantsAsyncLoad();
}

void USonheimTableManagerSubsystem::RequestCoreVariantsAsyncLoad()
{
	AreaObjectVariantPathMap.Reset();
	ItemVariantPathMap.Reset();
	ResourceObjectVariantPathMap.Reset();
	ContainerVariantPathMap.Reset();
	MonsterDexVariantPathMap.Reset();

	// Core5 이관 필드는 Variant 에셋에서만 반영한다.
	for (TPair<int32, FAreaObjectData>& Pair : AreaObjectDataMap)
	{
		Pair.Value.AreaObjectIcon = nullptr;
	}

	for (TPair<int32, FItemData>& Pair : ItemDataMap)
	{
		Pair.Value.ItemIcon = nullptr;
		Pair.Value.ItemMesh = nullptr;
		Pair.Value.MeshScale = FVector(1.0f);
	}

	for (TPair<int32, FResourceObjectData>& Pair : ResourceObjectDataMap)
	{
		Pair.Value.HarvestEffect = nullptr;
		Pair.Value.DestroyEffect = nullptr;
		Pair.Value.ResourceMesh = nullptr;
		Pair.Value.MeshScale = FVector(1.0f);
	}

	for (TPair<int32, FContainerData>& Pair : ContainerDataMap)
	{
		Pair.Value.ContainerMesh = nullptr;
		Pair.Value.OpenSound = nullptr;
		Pair.Value.CloseSound = nullptr;
	}

	for (TPair<int32, FMonsterDexData>& Pair : MonsterDexDataMap)
	{
		Pair.Value.Icon = nullptr;
	}

	const UAssetManager& AssetManager = UAssetManager::Get();
	TSet<FSoftObjectPath> UniqueVariantPaths;

	CollectVariantAssetPaths<FAreaObjectData>(
		AssetManager,
		TEXT("AreaObject"),
		AreaObjectVariantAssetType,
		AreaObjectDataMap,
		[](const FAreaObjectData& Row) { return Row.VariantId; },
		AreaObjectVariantPathMap,
		UniqueVariantPaths);

	CollectVariantAssetPaths<FItemData>(
		AssetManager,
		TEXT("Item"),
		ItemVariantAssetType,
		ItemDataMap,
		[](const FItemData& Row) { return Row.VariantId; },
		ItemVariantPathMap,
		UniqueVariantPaths);

	CollectVariantAssetPaths<FResourceObjectData>(
		AssetManager,
		TEXT("ResourceObject"),
		ResourceObjectVariantAssetType,
		ResourceObjectDataMap,
		[](const FResourceObjectData& Row) { return Row.VariantId; },
		ResourceObjectVariantPathMap,
		UniqueVariantPaths);

	CollectVariantAssetPaths<FContainerData>(
		AssetManager,
		TEXT("Container"),
		ContainerVariantAssetType,
		ContainerDataMap,
		[](const FContainerData& Row) { return Row.VariantId; },
		ContainerVariantPathMap,
		UniqueVariantPaths);

	CollectVariantAssetPaths<FMonsterDexData>(
		AssetManager,
		TEXT("MonsterDex"),
		MonsterDexVariantAssetType,
		MonsterDexDataMap,
		[](const FMonsterDexData& Row) { return Row.VariantId; },
		MonsterDexVariantPathMap,
		UniqueVariantPaths);

	checkf(!UniqueVariantPaths.IsEmpty(), TEXT("[TableManager] Core variant path registry is empty."));

	TArray<FSoftObjectPath> Paths = UniqueVariantPaths.Array();
	CoreVariantLoadHandle = RuntimeDataStreamableManager.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &USonheimTableManagerSubsystem::OnCoreVariantsLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!CoreVariantLoadHandle.IsValid())
	{
		bHasFailed = true;
		ensureAlwaysMsgf(false, TEXT("[TableManager] Failed to request async core variant load handle."));
		checkf(false, TEXT("[TableManager] Failed to request async core variant load handle."));
	}
}

void USonheimTableManagerSubsystem::OnCoreVariantsLoaded()
{
	for (TPair<int32, FAreaObjectData>& Pair : AreaObjectDataMap)
	{
		const FSoftObjectPath* VariantPath = AreaObjectVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing AreaObject variant path. DomainId=%d"), Pair.Key);
		USonheimAreaObjectVariantData* Variant = ResolveVariantAsset<USonheimAreaObjectVariantData>(*VariantPath, TEXT("AreaObject"), Pair.Key);
		Pair.Value.AreaObjectIcon = Variant->AreaObjectIcon;
	}

	for (TPair<int32, FItemData>& Pair : ItemDataMap)
	{
		const FSoftObjectPath* VariantPath = ItemVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing Item variant path. DomainId=%d"), Pair.Key);
		USonheimItemVariantData* Variant = ResolveVariantAsset<USonheimItemVariantData>(*VariantPath, TEXT("Item"), Pair.Key);
		Pair.Value.ItemIcon = Variant->ItemIcon;
		Pair.Value.ItemMesh = Variant->ItemMesh;
		Pair.Value.MeshScale = Variant->MeshScale;
	}

	for (TPair<int32, FResourceObjectData>& Pair : ResourceObjectDataMap)
	{
		const FSoftObjectPath* VariantPath = ResourceObjectVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing ResourceObject variant path. DomainId=%d"), Pair.Key);
		USonheimResourceObjectVariantData* Variant = ResolveVariantAsset<USonheimResourceObjectVariantData>(*VariantPath, TEXT("ResourceObject"), Pair.Key);
		Pair.Value.HarvestEffect = Variant->HarvestEffect;
		Pair.Value.DestroyEffect = Variant->DestroyEffect;
		Pair.Value.ResourceMesh = Variant->ResourceMesh;
		Pair.Value.MeshScale = Variant->MeshScale;
	}

	for (TPair<int32, FContainerData>& Pair : ContainerDataMap)
	{
		const FSoftObjectPath* VariantPath = ContainerVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing Container variant path. DomainId=%d"), Pair.Key);
		USonheimContainerVariantData* Variant = ResolveVariantAsset<USonheimContainerVariantData>(*VariantPath, TEXT("Container"), Pair.Key);
		Pair.Value.ContainerMesh = Variant->ContainerMesh;
		Pair.Value.OpenSound = Variant->OpenSound;
		Pair.Value.CloseSound = Variant->CloseSound;
	}

	for (TPair<int32, FMonsterDexData>& Pair : MonsterDexDataMap)
	{
		const FSoftObjectPath* VariantPath = MonsterDexVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing MonsterDex variant path. DomainId=%d"), Pair.Key);
		USonheimMonsterDexVariantData* Variant = ResolveVariantAsset<USonheimMonsterDexVariantData>(*VariantPath, TEXT("MonsterDex"), Pair.Key);
		Pair.Value.Icon = Variant->Icon;
	}

	RequestUIVariantsAsyncLoad();
}

void USonheimTableManagerSubsystem::RequestUIVariantsAsyncLoad()
{
	UIWidgetDefVariantPathMap.Reset();
	UIPresetVariantPathMap.Reset();
	UIWidgetDefVariantMap.Reset();
	UIPresetVariantMap.Reset();

	// E03 hard cutover: UI class reference is hydrated from variant only.
	for (TPair<FName, FUIWidgetDefRow>& Pair : UIWidgetDefMap)
	{
		Pair.Value.WidgetClass = nullptr;
	}

	const UAssetManager& AssetManager = UAssetManager::Get();
	TSet<FSoftObjectPath> UniqueVariantPaths;

	CollectVariantAssetPathsByName<FUIWidgetDefRow>(
		AssetManager,
		TEXT("UIWidgetDef"),
		UIWidgetDefVariantAssetType,
		UIWidgetDefMap,
		[](const FUIWidgetDefRow& Row) { return Row.VariantId; },
		UIWidgetDefVariantPathMap,
		UniqueVariantPaths);

	CollectVariantAssetPathsByName<FUIWidgetPresetRow>(
		AssetManager,
		TEXT("UIPreset"),
		UIPresetVariantAssetType,
		UIPresetMap,
		[](const FUIWidgetPresetRow& Row) { return Row.VariantId; },
		UIPresetVariantPathMap,
		UniqueVariantPaths);

	if (UniqueVariantPaths.IsEmpty())
	{
		OnUIVariantsLoaded();
		return;
	}

	TArray<FSoftObjectPath> Paths = UniqueVariantPaths.Array();
	UIVariantLoadHandle = RuntimeDataStreamableManager.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &USonheimTableManagerSubsystem::OnUIVariantsLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!UIVariantLoadHandle.IsValid())
	{
		bHasFailed = true;
		ensureAlwaysMsgf(false, TEXT("[TableManager] Failed to request async UI variant load handle."));
		checkf(false, TEXT("[TableManager] Failed to request async UI variant load handle."));
	}
}

void USonheimTableManagerSubsystem::OnUIVariantsLoaded()
{
	for (TPair<FName, FUIWidgetDefRow>& Pair : UIWidgetDefMap)
	{
		const FSoftObjectPath* VariantPath = UIWidgetDefVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing UIWidgetDef variant path. UIId=%s"), *Pair.Key.ToString());

		USonheimUIWidgetDefVariantData* Variant = ResolveVariantAsset<USonheimUIWidgetDefVariantData>(
			*VariantPath,
			TEXT("UIWidgetDef"),
			Pair.Key);
		UIWidgetDefVariantMap.Add(Pair.Key, Variant);
		Pair.Value.WidgetClass = Variant->WidgetClass;
		checkf(!Pair.Value.WidgetClass.IsNull(),
			TEXT("[TableManager] UIWidgetDef variant has null WidgetClass. UIId=%s VariantId=%s"),
			*Pair.Key.ToString(),
			*Pair.Value.VariantId.ToString());
	}

	for (TPair<FName, FUIWidgetPresetRow>& Pair : UIPresetMap)
	{
		const FSoftObjectPath* VariantPath = UIPresetVariantPathMap.Find(Pair.Key);
		checkf(VariantPath, TEXT("[TableManager] Missing UIPreset variant path. PresetId=%s"), *Pair.Key.ToString());

		USonheimUIPresetVariantData* Variant = ResolveVariantAsset<USonheimUIPresetVariantData>(
			*VariantPath,
			TEXT("UIPreset"),
			Pair.Key);
		UIPresetVariantMap.Add(Pair.Key, Variant);
	}

	bIsReady = true;
	bHasFailed = false;

	UE_LOG(
		SONHEIM,
		Log,
		TEXT("[TableManager] Runtime Data Ready. LoadOrder=DataTable->CoreVariant->UIVariant->Ready. CoreVariants: AreaObject=%d Item=%d Resource=%d Container=%d MonsterDex=%d UIVariants: WidgetDef=%d UIPreset=%d"),
		AreaObjectVariantPathMap.Num(),
		ItemVariantPathMap.Num(),
		ResourceObjectVariantPathMap.Num(),
		ContainerVariantPathMap.Num(),
		MonsterDexVariantPathMap.Num(),
		UIWidgetDefVariantPathMap.Num(),
		UIPresetVariantPathMap.Num());

	OnRuntimeDataReady.Broadcast();
	BuildSelectedAssetPreload();
}

void USonheimTableManagerSubsystem::BuildSelectedAssetPreload()
{
	TSet<FSoftObjectPath> UniquePaths;

	for (const TPair<int32, FAreaObjectData>& Pair : AreaObjectDataMap)
	{
		AddSoftPathIfValid(Pair.Value.AreaObjectIcon, UniquePaths);
	}

	for (const TPair<int32, FMonsterDexData>& Pair : MonsterDexDataMap)
	{
		AddSoftPathIfValid(Pair.Value.Icon, UniquePaths);
	}

	for (const TPair<int32, FItemData>& Pair : ItemDataMap)
	{
		AddSoftPathIfValid(Pair.Value.ItemIcon, UniquePaths);
	}

	for (const TPair<int32, FContainerData>& Pair : ContainerDataMap)
	{
		AddSoftPathIfValid(Pair.Value.ContainerMesh, UniquePaths);
	}

	for (const TPair<int32, FResourceObjectData>& Pair : ResourceObjectDataMap)
	{
		AddSoftPathIfValid(Pair.Value.ResourceMesh, UniquePaths);
	}

	int32 UIWidgetPreloadCount = 0;
	for (const TPair<FName, TObjectPtr<USonheimUIWidgetDefVariantData>>& Pair : UIWidgetDefVariantMap)
	{
		const USonheimUIWidgetDefVariantData* Variant = Pair.Value;
		if (!Variant || !Variant->bPreloadWidgetClass)
		{
			continue;
		}

		AddSoftClassPathIfValid(Variant->WidgetClass, UniquePaths);
		++UIWidgetPreloadCount;
	}

	if (UniquePaths.IsEmpty())
	{
		bSelectedAssetPreloadComplete = true;
		return;
	}

	TArray<FSoftObjectPath> Paths = UniquePaths.Array();
	SelectedAssetPreloadHandle = RuntimeDataStreamableManager.RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &USonheimTableManagerSubsystem::OnSelectedAssetPreloadComplete),
		FStreamableManager::AsyncLoadHighPriority);

	if (!SelectedAssetPreloadHandle.IsValid())
	{
		UE_LOG(SONHEIM, Warning, TEXT("[TableManager] Selected asset preload handle is invalid."));
		bSelectedAssetPreloadComplete = true;
		return;
	}

	UE_LOG(SONHEIM, Log, TEXT("[TableManager] Selected preload requested. UIWidgetClass=%d"), UIWidgetPreloadCount);
}

void USonheimTableManagerSubsystem::OnSelectedAssetPreloadComplete()
{
	bSelectedAssetPreloadComplete = true;
	UE_LOG(SONHEIM, Log, TEXT("[TableManager] Selected soft asset preload complete."));
}

void USonheimTableManagerSubsystem::MarkPreReadyAccess(const TCHAR* Context) const
{
	++PreReadyAccessCount;

	const FName ContextName(Context);
	if (!PreReadyWarningContexts.Contains(ContextName))
	{
		PreReadyWarningContexts.Add(ContextName);
		ensureAlwaysMsgf(false, TEXT("[TableManager] Pre-ready access detected: %s"), Context);
	}
}

bool USonheimTableManagerSubsystem::IsRuntimeDataAccessible(const TCHAR* Context) const
{
	if (bHasFailed)
	{
		checkf(false, TEXT("[TableManager] Runtime data is in failed state."));
		return false;
	}

	if (!bIsReady)
	{
		MarkPreReadyAccess(Context);
		return false;
	}

	return true;
}

const FAreaObjectData* USonheimTableManagerSubsystem::FindAreaObject(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindAreaObject"))) return nullptr;
	return AreaObjectDataMap.Find(Id);
}

const FSkillData* USonheimTableManagerSubsystem::FindSkill(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindSkill"))) return nullptr;
	return SkillDataMap.Find(Id);
}

const FSkillBagData* USonheimTableManagerSubsystem::FindSkillBag(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindSkillBag"))) return nullptr;
	return SkillBagDataMap.Find(Id);
}

const FResourceObjectData* USonheimTableManagerSubsystem::FindResourceObject(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindResourceObject"))) return nullptr;
	return ResourceObjectDataMap.Find(Id);
}

const FItemData* USonheimTableManagerSubsystem::FindItem(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindItem"))) return nullptr;
	return ItemDataMap.Find(Id);
}

const FLevelData* USonheimTableManagerSubsystem::FindLevel(int32 Level) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindLevel"))) return nullptr;
	return LevelDataMap.Find(Level);
}

const FContainerData* USonheimTableManagerSubsystem::FindContainer(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindContainer"))) return nullptr;
	return ContainerDataMap.Find(Id);
}

const FQuestData* USonheimTableManagerSubsystem::FindQuest(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindQuest"))) return nullptr;
	return QuestDataMap.Find(Id);
}

const FRewardDef* USonheimTableManagerSubsystem::FindReward(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindReward"))) return nullptr;
	return RewardDataMap.Find(Id);
}

const FMonsterDexData* USonheimTableManagerSubsystem::FindMonsterDex(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindMonsterDex"))) return nullptr;
	return MonsterDexDataMap.Find(Id);
}

const FDropTableRow* USonheimTableManagerSubsystem::FindDropTable(int32 Id) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindDropTable"))) return nullptr;
	return DropTableDataMap.Find(Id);
}

const FUIWidgetDefRow* USonheimTableManagerSubsystem::FindUIWidgetDef(FName UIId) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindUIWidgetDef"))) return nullptr;
	return UIWidgetDefMap.Find(UIId);
}

const FUIWidgetPresetRow* USonheimTableManagerSubsystem::FindUIPreset(FName PresetId) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindUIPreset"))) return nullptr;
	return UIPresetMap.Find(PresetId);
}

const USonheimUIPresetVariantData* USonheimTableManagerSubsystem::FindUIPresetVariant(FName PresetId) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindUIPresetVariant"))) return nullptr;
	const TObjectPtr<USonheimUIPresetVariantData>* Variant = UIPresetVariantMap.Find(PresetId);
	return Variant ? Variant->Get() : nullptr;
}

const USonheimUIWidgetDefVariantData* USonheimTableManagerSubsystem::FindUIWidgetDefVariant(FName UIId) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindUIWidgetDefVariant"))) return nullptr;
	const TObjectPtr<USonheimUIWidgetDefVariantData>* Variant = UIWidgetDefVariantMap.Find(UIId);
	return Variant ? Variant->Get() : nullptr;
}

const TSoftObjectPtr<USoundBase>* USonheimTableManagerSubsystem::FindSound(int32 SoundID) const
{
	if (!IsRuntimeDataAccessible(TEXT("FindSound"))) return nullptr;
	return SoundDataMap.Find(SoundID);
}
