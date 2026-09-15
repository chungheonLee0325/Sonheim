#include "DungeonAssetSubsystem.h"
#include "Engine/AssetManager.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonSpawnRuleDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonPresentationDataAsset.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

FGuid UDungeonAssetSubsystem::LoadDefinition(FPrimaryAssetId AssetId, TFunction<void(UDungeonDefinitionDataAsset*, const FString&)> Completion, bool bLoadGameplay)
{
	const FGuid Id = FGuid::NewGuid();
	FDungeonAssetRequest& Request = Requests.Add(Id);
	Request.AssetId = AssetId;
	Request.bLoadGameplay = bLoadGameplay;
	Request.Completion = MoveTemp(Completion);
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(AssetId);
	// Each request owns its streamable handles. Cancelling one PIE world must not cancel another world's load.
	if (!Path.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Id]() { Finish(Id, TEXT("Primary Asset ID could not be resolved.")); }));
		return Id;
	}
	Request.Handles.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,
		FStreamableDelegate::CreateUObject(this, &UDungeonAssetSubsystem::OnDefinitionLoaded, Id)));
	UE_LOG(SONHEIM, Log, TEXT("[DungeonAssets] Request=%s Asset=%s Path=%s"), *Id.ToString(), *AssetId.ToString(), *Path.ToString());
	return Id;
}

void UDungeonAssetSubsystem::OnDefinitionLoaded(FGuid Id)
{
	auto* Request = Requests.Find(Id);
	if (!Request) return;
	auto* Definition = Cast<UDungeonDefinitionDataAsset>(UAssetManager::Get().GetPrimaryAssetPath(Request->AssetId).ResolveObject());
	if (!Definition || Definition->GetPrimaryAssetId() != Request->AssetId) { Finish(Id, TEXT("Definition type/identity mismatch.")); return; }
	Definitions.Add(Id, Definition);
	TArray<FSoftObjectPath> Paths;
	Paths.AddUnique(Definition->Presentation.ToSoftObjectPath());
	for (const auto& Stage : Definition->Stages)
		for (const auto& Rule : Stage.EventRules)
			for (const auto& Action : Rule.Actions)
				if (Request->bLoadGameplay && Action.Type == EDungeonStageAction::SpawnGroup && !Action.SpawnRule.IsNull()) Paths.AddUnique(Action.SpawnRule.ToSoftObjectPath());
	Paths.RemoveAll([](const FSoftObjectPath& Path) { return !Path.IsValid(); });
	if (Paths.IsEmpty()) { Finish(Id, TEXT("Definition has no presentation/spawn references.")); return; }
	Request->Handles.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,
		FStreamableDelegate::CreateUObject(this, &UDungeonAssetSubsystem::OnReferencesLoaded, Id)));
}

void UDungeonAssetSubsystem::OnReferencesLoaded(FGuid Id)
{
	auto* Request = Requests.Find(Id);
	auto* Definition = Definitions.FindRef(Id).Get();
	if (!Request || !Definition) return;
	TArray<FString> Errors, Warnings;
	if (!Definition->ValidateDefinition(Errors, Warnings) || !Definition->Presentation.Get()) { Finish(Id, TEXT("Definition validation/references failed: ") + FString::Join(Errors, TEXT("; "))); return; }
	if (!Request->bLoadGameplay) { Finish(Id, TEXT("")); return; }
	TArray<FSoftObjectPath> Paths;
	for (const auto& Stage : Definition->Stages)
		for (const auto& Rule : Stage.EventRules)
			for (const auto& Action : Rule.Actions)
				if (Action.Type == EDungeonStageAction::SpawnGroup)
				{
					const auto* SpawnRule = Action.SpawnRule.Get();
					if (!SpawnRule || SpawnRule->MonsterClass.IsNull()) { Finish(Id, TEXT("Missing spawn rule/class.")); return; }
					Paths.AddUnique(SpawnRule->MonsterClass.ToSoftObjectPath());
				}
	if (Paths.IsEmpty()) { Finish(Id, TEXT("")); return; }
	Request->Handles.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate::CreateWeakLambda(this, [this, Id]()
	{
		auto* Def = Definitions.FindRef(Id).Get();
		if (!Requests.Contains(Id) || !Def) return;
		for (const auto& Stage : Def->Stages)
			for (const auto& Rule : Stage.EventRules)
				for (const auto& Action : Rule.Actions)
					if (Action.Type == EDungeonStageAction::SpawnGroup && (!Action.SpawnRule.Get() || !Action.SpawnRule->MonsterClass.Get()))
					{ Finish(Id, TEXT("Monster class did not load.")); return; }
		Finish(Id, TEXT(""));
	})));
}

void UDungeonAssetSubsystem::Finish(FGuid Id, const FString& Error)
{
	auto* Request = Requests.Find(Id);
	if (!Request) return;
	auto Completion = MoveTemp(Request->Completion);
	auto* Definition = Definitions.FindRef(Id).Get();
	UE_LOG(SONHEIM, Log, TEXT("[DungeonAssets] Ready=%d Request=%s Error=%s"), Error.IsEmpty(), *Id.ToString(), *Error);
	if (Completion) Completion(Error.IsEmpty() ? Definition : nullptr, Error);
	if (!Error.IsEmpty()) Release(Id);
}

void UDungeonAssetSubsystem::Release(FGuid Id)
{
	Requests.Remove(Id);
	Definitions.Remove(Id);
}

void UDungeonAssetSubsystem::Deinitialize()
{
	Requests.Empty(); Definitions.Empty();
	Super::Deinitialize();
}
