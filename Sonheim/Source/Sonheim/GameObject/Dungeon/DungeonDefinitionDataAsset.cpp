#include "DungeonDefinitionDataAsset.h"
#include "DungeonSpawnRuleDataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UDungeonDefinitionDataAsset::GetPrimaryAssetId() const
{
	return DefinitionId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(TEXT("DungeonDefinition"), DefinitionId);
}

const FDungeonStageDefinition* UDungeonDefinitionDataAsset::FindStage(FName Id) const
{
	return Stages.FindByPredicate([Id](const FDungeonStageDefinition& Stage) { return Stage.StageId == Id; });
}

bool UDungeonDefinitionDataAsset::ValidateDefinition(TArray<FString>& Errors, TArray<FString>& Warnings) const
{
	Errors.Reset(); Warnings.Reset();
	if (DefinitionId.IsNone()) Errors.Add(TEXT("DefinitionId is required."));
	if (StartStageId.IsNone() || !FindStage(StartStageId)) Errors.Add(TEXT("Invalid StartStageId."));
	if (Presentation.IsNull()) Errors.Add(TEXT("Presentation is required."));
	TSet<FName> Ids, TransitionIds;
	TMap<FName, TArray<FName>> Edges;
	struct FProducer { FName Stage; EDungeonStageEvent Event; FName Source; bool bBoss; };
	TMap<FName, TArray<FProducer>> ProducersByGroup;
	for (const FDungeonStageDefinition& Stage : Stages)
	{
		if (Stage.StageId.IsNone() || Ids.Contains(Stage.StageId)) Errors.Add(FString::Printf(TEXT("Duplicate/empty StageId: %s"), *Stage.StageId.ToString()));
		Ids.Add(Stage.StageId);
		TSet<FString> RuleKeys;
		int32 TransitionCount = 0;
		for (const FDungeonStageEventRule& Rule : Stage.EventRules)
		{
			const FString Key = FString::Printf(TEXT("%d:%s"), int32(Rule.Event), *Rule.SourceId.ToString());
			if (RuleKeys.Contains(Key)) Errors.Add(Stage.StageId.ToString() + TEXT(": conflicting event rules ") + Key);
			RuleKeys.Add(Key);
			for (const FDungeonStageAction& Action : Rule.Actions)
			{
				if (Action.Type == EDungeonStageAction::SpawnGroup)
				{
					if (Action.GroupId.IsNone() || Action.PointSetId.IsNone() || Action.SpawnRule.IsNull()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid spawn action."));
					ProducersByGroup.FindOrAdd(Action.GroupId).Add({Stage.StageId, Rule.Event, Rule.SourceId, Action.bBossGroup});
					if (const auto* SpawnRule = Action.SpawnRule.Get())
					{
						if (SpawnRule->Count < 1 || SpawnRule->Count > 16 || SpawnRule->MonsterClass.IsNull()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid spawn rule."));
					}
				}
				if ((Action.Type == EDungeonStageAction::SetRunTag || Action.Type == EDungeonStageAction::ClearRunTag) && !Action.RunTag.IsValid()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid RunTag action."));
				if (Action.Type == EDungeonStageAction::EmitEvent && Action.Event != EDungeonStageEvent::StageEntered) Errors.Add(TEXT("EmitEvent only permits StageEntered. Combat/interaction facts require trusted producers."));
				if (Action.Type == EDungeonStageAction::EmitEvent && Rule.Event == Action.Event && Rule.SourceId == Action.EventSourceId) Errors.Add(TEXT("Immediate event self-loop."));
			}
			for (int32 Index = 0; Index < Rule.Transitions.Num(); ++Index)
			{
				const auto& Transition = Rule.Transitions[Index];
				++TransitionCount;
				if (Transition.TransitionId.IsNone() || TransitionIds.Contains(Transition.TransitionId)) Errors.Add(TEXT("Duplicate/empty TransitionId."));
				TransitionIds.Add(Transition.TransitionId);
				if (!FindStage(Transition.NextStageId)) Errors.Add(TEXT("Missing NextStageId: ") + Transition.NextStageId.ToString());
				Edges.FindOrAdd(Stage.StageId).Add(Transition.NextStageId);
				if (Transition.Conditions.IsEmpty()) Errors.Add(TEXT("Transition requires explicit Conditions (use Always for fallback)."));
				bool bAlways = !Transition.Conditions.IsEmpty();
				for (const auto& Condition : Transition.Conditions)
				{
					bAlways &= Condition.Type == EDungeonStageCondition::Always && !Condition.bNegate;
					if (Condition.Type == EDungeonStageCondition::HasRunTag && !Condition.RunTag.IsValid()) Errors.Add(TEXT("HasRunTag requires a valid tag."));
					if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted && Condition.GroupId.IsNone()) Errors.Add(TEXT("SpawnGroupCompleted requires GroupId."));
				}
				if (bAlways && Index != Rule.Transitions.Num() - 1) Errors.Add(TEXT("Always branch shadows subsequent transitions."));
			}
		}
		if (Stage.TerminalOutcome == EDungeonTerminalOutcome::None && TransitionCount == 0) Errors.Add(Stage.StageId.ToString() + TEXT(": non-terminal has no transition."));
		if (Stage.TerminalOutcome != EDungeonTerminalOutcome::None && !Stage.EventRules.IsEmpty()) Errors.Add(Stage.StageId.ToString() + TEXT(": terminal rules would never execute."));
	}
	TSet<FName> Visiting, Visited;
	TFunction<void(FName)> Visit = [&](FName Id)
	{
		if (Visiting.Contains(Id)) { Errors.Add(TEXT("Self/cyclic transition at ") + Id.ToString()); return; }
		if (Visited.Contains(Id)) return;
		Visiting.Add(Id);
		for (FName Next : Edges.FindRef(Id)) Visit(Next);
		Visiting.Remove(Id); Visited.Add(Id);
	};
	Visit(StartStageId);
	const TSet<FName> Reachable = Visited;
	for (const auto& Stage : Stages)
	{
		if (!Reachable.Contains(Stage.StageId)) Warnings.Add(TEXT("Unreachable stage: ") + Stage.StageId.ToString());
		Visit(Stage.StageId); // Cycles in disconnected authoring are also errors.
	}
	auto CanPrecede = [&](FName Producer, FName Consumer)
	{
		TSet<FName> Seen; TArray<FName> Pending{Producer};
		while (!Pending.IsEmpty())
		{
			FName Id = Pending.Pop(); if (Id == Consumer) return true;
			if (Seen.Contains(Id)) continue;
			Seen.Add(Id); Pending.Append(Edges.FindRef(Id));
		}
		return false;
	};
	for (const auto& Stage : Stages)
	{
		for (const auto& Rule : Stage.EventRules)
		{
			for (const auto& Transition : Rule.Transitions)
				for (const auto& Condition : Transition.Conditions)
					if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted)
					{
						const auto* Producers = ProducersByGroup.Find(Condition.GroupId);
						if (!Producers) Errors.Add(TEXT("No producer for condition group: ") + Condition.GroupId.ToString());
						else if (!Condition.bNegate && Reachable.Contains(Stage.StageId))
						{
							const bool bHasPath = Producers->ContainsByPredicate([&](const FProducer& Producer)
							{ return Reachable.Contains(Producer.Stage) && CanPrecede(Producer.Stage, Stage.StageId); });
							if (!bHasPath) Errors.Add(TEXT("No preceding producer for condition group: ") + Condition.GroupId.ToString());
						}
					}
			if (Rule.Event != EDungeonStageEvent::WaveCompleted && Rule.Event != EDungeonStageEvent::BossDefeated) continue;
			const auto Producers = ProducersByGroup.FindRef(Rule.SourceId);
			bool bHasPath = false;
			for (const auto& Producer : Producers)
			{
				if (Producer.bBoss != (Rule.Event == EDungeonStageEvent::BossDefeated)) continue;
				if (Producer.Event == Rule.Event && Producer.Source == Rule.SourceId) continue; // Cannot spawn only after its own completion.
				bHasPath |= Reachable.Contains(Producer.Stage) && CanPrecede(Producer.Stage, Stage.StageId);
			}
			if (Reachable.Contains(Stage.StageId) && !bHasPath) Errors.Add(Stage.StageId.ToString() + TEXT(": no preceding spawn path for ") + Rule.SourceId.ToString());
		}
	}
	return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UDungeonDefinitionDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	TArray<FString> Errors, Warnings;
	ValidateDefinition(Errors, Warnings);
	// Editor dependency checks may load assets. Runtime/presentation validation must not.
	for (const auto& Stage : Stages)
		for (const auto& Rule : Stage.EventRules)
			for (const auto& Action : Rule.Actions)
				if (Action.Type == EDungeonStageAction::SpawnGroup && !Action.SpawnRule.IsNull())
				{
					const auto* SpawnRule = Action.SpawnRule.LoadSynchronous();
					if (!SpawnRule || SpawnRule->Count < 1 || SpawnRule->Count > 16 || SpawnRule->MonsterClass.IsNull())
						Errors.Add(Stage.StageId.ToString() + TEXT(": missing or invalid spawn rule dependency."));
				}
	for (const auto& Error : Errors) Context.AddError(FText::FromString(Error));
	for (const auto& Warning : Warnings) Context.AddWarning(FText::FromString(Warning));
	return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
