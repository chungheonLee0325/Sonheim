#include "DungeonDefinitionDataAsset.h"
#include "DungeonSpawnRuleDataAsset.h"
#include "Sonheim/Utilities/LogMacro.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

namespace
{
	FString EnumName(const UEnum* Enum, int64 Value)
	{
		return Enum ? Enum->GetNameStringByValue(Value) : FString::FromInt(Value);
	}

	FString ConditionText(const FDungeonStageCondition& Condition)
	{
		FString Text = EnumName(StaticEnum<EDungeonStageCondition>(), int64(Condition.Type));
		if (Condition.Type == EDungeonStageCondition::HasRunTag) Text += TEXT(" ") + Condition.RunTag.GetTagName().ToString();
		if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted) Text += TEXT(" ") + Condition.GroupId.ToString();
		return Condition.bNegate ? TEXT("not ") + Text : Text;
	}
}

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
				if (Action.Type == EDungeonStageAction::GrantReward && (Action.RewardItemId <= 0 || Action.RewardCount < 1 || Action.RewardCount > 99)) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid reward action."));
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

FString UDungeonDefinitionDataAsset::BuildStageGraph() const
{
	// Mermaid, because it renders in the places a design document already lives and stays readable as plain text.
	FString Text = FString::Printf(TEXT("flowchart TD\n  %%%% %s, start %s\n"), *DefinitionId.ToString(), *StartStageId.ToString());
	for (const FDungeonStageDefinition& Stage : Stages)
	{
		const FString Id = Stage.StageId.ToString();
		if (Stage.TerminalOutcome != EDungeonTerminalOutcome::None)
		{
			Text += FString::Printf(TEXT("  %s[[\"%s %s\"]]\n"), *Id, *Id,
				Stage.TerminalOutcome == EDungeonTerminalOutcome::Success ? TEXT("성공") : TEXT("실패"));
		}
		for (const FDungeonStageEventRule& Rule : Stage.EventRules)
		{
			TArray<FString> Actions;
			for (const FDungeonStageAction& Action : Rule.Actions)
			{
				FString Entry = EnumName(StaticEnum<EDungeonStageAction>(), int64(Action.Type));
				if (Action.Type == EDungeonStageAction::SpawnGroup) Entry += TEXT(" ") + Action.GroupId.ToString();
				if (Action.Type == EDungeonStageAction::SetRunTag || Action.Type == EDungeonStageAction::ClearRunTag) Entry += TEXT(" ") + Action.RunTag.GetTagName().ToString();
				if (Action.Type == EDungeonStageAction::GrantReward) Entry += FString::Printf(TEXT(" %d x%d"), Action.RewardItemId, Action.RewardCount);
				Actions.Add(Entry);
			}
			const FString Event = EnumName(StaticEnum<EDungeonStageEvent>(), int64(Rule.Event)) +
				(Rule.SourceId.IsNone() ? FString() : TEXT(" ") + Rule.SourceId.ToString());
			if (!Actions.IsEmpty()) Text += FString::Printf(TEXT("  %%%% %s on %s: %s\n"), *Id, *Event, *FString::Join(Actions, TEXT(", ")));
			for (const FDungeonStageTransition& Transition : Rule.Transitions)
			{
				TArray<FString> Conditions;
				for (const FDungeonStageCondition& Condition : Transition.Conditions) Conditions.Add(ConditionText(Condition));
				FString Label = Event + TEXT(" / ") + FString::Join(Conditions, TEXT(" and "));
				if (!Transition.BranchId.IsNone()) Label += TEXT(" => ") + Transition.BranchId.ToString();
				Text += FString::Printf(TEXT("  %s -->|\"%s\"| %s\n"), *Id, *Label, *Transition.NextStageId.ToString());
			}
		}
	}
	TArray<FString> Errors, Warnings;
	ValidateDefinition(Errors, Warnings);
	Text += FString::Printf(TEXT("  %%%% 오류 %d, 경고 %d\n"), Errors.Num(), Warnings.Num());
	return Text;
}

#if WITH_EDITOR
namespace
{
	void Notify(const FString& Message, bool bSuccess)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 6.f;
		if (const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}
}

void UDungeonDefinitionDataAsset::ValidateNow()
{
	TArray<FString> Errors, Warnings;
	const bool bValid = ValidateDefinition(Errors, Warnings);
	for (const FString& Error : Errors) UE_LOG(SONHEIM, Error, TEXT("[DungeonDefinition] %s: %s"), *GetName(), *Error);
	for (const FString& Warning : Warnings) UE_LOG(SONHEIM, Warning, TEXT("[DungeonDefinition] %s: %s"), *GetName(), *Warning);
	// The counts go on screen; the lines themselves go to the output log, where they can be read and copied.
	Notify(FString::Printf(TEXT("%s: 오류 %d, 경고 %d%s"), *GetName(), Errors.Num(), Warnings.Num(),
		Errors.Num() + Warnings.Num() > 0 ? TEXT(" (Output Log 참고)") : TEXT("")), bValid);
}

void UDungeonDefinitionDataAsset::CopyStageGraph()
{
	const FString Graph = BuildStageGraph();
	FPlatformApplicationMisc::ClipboardCopy(*Graph);
	UE_LOG(SONHEIM, Log, TEXT("[DungeonDefinition] %s stage graph\n%s"), *GetName(), *Graph);
	Notify(FString::Printf(TEXT("%s 스테이지 그래프를 클립보드에 복사했다 (%d자)"), *GetName(), Graph.Len()), true);
}
#endif
