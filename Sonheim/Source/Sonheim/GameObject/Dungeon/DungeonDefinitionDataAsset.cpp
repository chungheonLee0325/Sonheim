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

	/** A tag without its dungeon's part, such as Stage.Combat: what a reader of one dungeon's graph needs. */
	FString Short(const FGameplayTag& Tag, const FGameplayTag& Root)
	{
		FString Name = Tag.ToString();
		const FString Prefix = Root.ToString() + TEXT(".");
		if (Root.IsValid() && Name.StartsWith(Prefix)) Name.RightChopInline(Prefix.Len());
		return Name;
	}

	/** Mermaid node ids take no dots. */
	FString Node(const FGameplayTag& Tag, const FGameplayTag& Root)
	{
		return Short(Tag, Root).Replace(TEXT("."), TEXT("_"));
	}

	FString ConditionText(const FDungeonStageCondition& Condition, const FGameplayTag& Root)
	{
		FString Text = EnumName(StaticEnum<EDungeonStageCondition>(), int64(Condition.Type));
		if (Condition.Type == EDungeonStageCondition::HasRunTag) Text += TEXT(" ") + Short(Condition.RunTag, Root);
		if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted) Text += TEXT(" ") + Short(Condition.GroupId, Root);
		return Condition.bNegate ? TEXT("not ") + Text : Text;
	}
}

FPrimaryAssetId UDungeonDefinitionDataAsset::GetPrimaryAssetId() const
{
	return DungeonId.IsValid() ? FPrimaryAssetId(TEXT("DungeonDefinition"), DungeonId.GetTagName()) : FPrimaryAssetId();
}

const FDungeonStageDefinition* UDungeonDefinitionDataAsset::FindStage(const FGameplayTag& Id) const
{
	return Stages.FindByPredicate([&Id](const FDungeonStageDefinition& Stage) { return Stage.StageId == Id; });
}

FName UDungeonDefinitionDataAsset::GradeFor(float ClearSeconds, int32 OptionalObjectivesDone) const
{
	const FDungeonGradeRule* Rule = GradeRules.FindByPredicate([&](const FDungeonGradeRule& Item)
	{
		return (Item.MaxClearSeconds <= 0.f || ClearSeconds <= Item.MaxClearSeconds) && OptionalObjectivesDone >= Item.RequiredOptionalObjectives;
	});
	return Rule ? Rule->Grade : NAME_None;
}

bool UDungeonDefinitionDataAsset::PicksBranch(const FGameplayTag& RunTag) const
{
	for (const FDungeonStageDefinition& Stage : Stages)
		for (const FDungeonStageEventRule& Rule : Stage.EventRules)
			for (const FDungeonStageTransition& Transition : Rule.Transitions)
				if (Transition.BranchId.IsValid() && Transition.Conditions.ContainsByPredicate([&RunTag](const FDungeonStageCondition& Condition)
					{ return Condition.Type == EDungeonStageCondition::HasRunTag && Condition.RunTag == RunTag; }))
					return true;
	return false;
}
bool UDungeonDefinitionDataAsset::ValidateDefinition(TArray<FString>& Errors, TArray<FString>& Warnings) const
{
	Errors.Reset(); Warnings.Reset();
	if (!DungeonId.IsValid()) Errors.Add(TEXT("DungeonId is required."));
	if (!StartStageId.IsValid() || !FindStage(StartStageId)) Errors.Add(TEXT("Invalid StartStageId."));
	if (Presentation.IsNull()) Errors.Add(TEXT("Presentation is required."));
	// Every name the graph uses belongs to its dungeon; a tag of another dungeon is a slip in a list that offers all of them.
	auto Own = [&](const FGameplayTag& Tag, const TCHAR* What)
	{
		if (Tag.IsValid() && DungeonId.IsValid() && !Tag.MatchesTag(DungeonId))
			Errors.Add(FString::Printf(TEXT("%s %s is not under %s."), What, *Tag.ToString(), *DungeonId.ToString()));
	};
	TSet<FGameplayTag> Ids;
	TSet<FName> TransitionIds;
	TMap<FGameplayTag, TArray<FGameplayTag>> Edges;
	struct FProducer { FGameplayTag Stage; EDungeonStageEvent Event; FGameplayTag Source; bool bBoss; };
	TMap<FGameplayTag, TArray<FProducer>> ProducersByGroup;
	for (const FDungeonStageDefinition& Stage : Stages)
	{
		if (!Stage.StageId.IsValid() || Ids.Contains(Stage.StageId)) Errors.Add(FString::Printf(TEXT("Duplicate/empty StageId: %s"), *Stage.StageId.ToString()));
		Ids.Add(Stage.StageId);
		Own(Stage.StageId, TEXT("StageId"));
		TSet<FString> RuleKeys;
		int32 TransitionCount = 0;
		const bool bHasTimeout = Stage.EventRules.ContainsByPredicate([](const auto& Rule) { return Rule.Event == EDungeonStageEvent::StageTimeout; });
		if (Stage.TimeLimitSeconds < 0.f) Errors.Add(Stage.StageId.ToString() + TEXT(": TimeLimitSeconds cannot be negative."));
		if (Stage.TimeLimitSeconds > 0.f && !bHasTimeout) Errors.Add(Stage.StageId.ToString() + TEXT(": a time limit needs a StageTimeout rule."));
		if (Stage.TimeLimitSeconds <= 0.f && bHasTimeout) Errors.Add(Stage.StageId.ToString() + TEXT(": a StageTimeout rule needs a time limit."));
		// The level is not known here; a barrier the level lacks is reported when a run starts.
		TSet<FGameplayTag> Barriers;
		for (const FGameplayTag& Barrier : Stage.SealedBarriers)
		{
			bool bRepeated = false;
			Barriers.Add(Barrier, &bRepeated);
			if (!Barrier.IsValid() || bRepeated) Errors.Add(Stage.StageId.ToString() + TEXT(": SealedBarriers needs each BarrierId once."));
			Own(Barrier, TEXT("Barrier"));
		}
		for (const FDungeonStageEventRule& Rule : Stage.EventRules)
		{
			const FString Key = FString::Printf(TEXT("%s:%s"), *EnumName(StaticEnum<EDungeonStageEvent>(), int64(Rule.Event)), *Rule.SourceId.ToString());
			if (RuleKeys.Contains(Key)) Errors.Add(Stage.StageId.ToString() + TEXT(": conflicting event rules ") + Key);
			// StageEntered and StageTimeout are raised by the stage itself, so a source on them would never match.
			if ((Rule.Event == EDungeonStageEvent::StageEntered || Rule.Event == EDungeonStageEvent::StageTimeout) && Rule.SourceId.IsValid())
				Errors.Add(Stage.StageId.ToString() + TEXT(": ") + Key + TEXT(" must have no SourceId."));
			// An interaction or a zone names the actor it comes from, and a capture the group, so a rule without that name would never match.
			if ((Rule.Event == EDungeonStageEvent::ActorInteracted || Rule.Event == EDungeonStageEvent::AreaEntered || Rule.Event == EDungeonStageEvent::MonsterCaptured) && !Rule.SourceId.IsValid())
				Errors.Add(Stage.StageId.ToString() + TEXT(": ") + Key + TEXT(" needs the SourceId of its actor or group."));
			Own(Rule.SourceId, TEXT("SourceId"));
			RuleKeys.Add(Key);
			for (const FDungeonStageAction& Action : Rule.Actions)
			{
				if (Action.Type == EDungeonStageAction::SpawnGroup)
				{
					if (!Action.GroupId.IsValid() || !Action.PointSetId.IsValid() || Action.SpawnRule.IsNull()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid spawn action."));
					Own(Action.GroupId, TEXT("GroupId"));
					Own(Action.PointSetId, TEXT("PointSetId"));
					ProducersByGroup.FindOrAdd(Action.GroupId).Add({Stage.StageId, Rule.Event, Rule.SourceId, Action.bBossGroup});
					if (const auto* SpawnRule = Action.SpawnRule.Get())
					{
						if (SpawnRule->Count < 1 || SpawnRule->Count > 16 || SpawnRule->MonsterClass.IsNull()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid spawn rule."));
					}
				}
				if ((Action.Type == EDungeonStageAction::SetRunTag || Action.Type == EDungeonStageAction::ClearRunTag))
				{
					if (!Action.RunTag.IsValid()) Errors.Add(Stage.StageId.ToString() + TEXT(": invalid RunTag action."));
					Own(Action.RunTag, TEXT("RunTag"));
				}
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
				Own(Transition.BranchId, TEXT("BranchId"));
				Edges.FindOrAdd(Stage.StageId).Add(Transition.NextStageId);
				if (Transition.Conditions.IsEmpty()) Errors.Add(TEXT("Transition requires explicit Conditions (use Always for fallback)."));
				bool bAlways = !Transition.Conditions.IsEmpty();
				for (const auto& Condition : Transition.Conditions)
				{
					bAlways &= Condition.Type == EDungeonStageCondition::Always && !Condition.bNegate;
					if (Condition.Type == EDungeonStageCondition::HasRunTag && !Condition.RunTag.IsValid()) Errors.Add(TEXT("HasRunTag requires a valid tag."));
					if (Condition.Type == EDungeonStageCondition::HasRunTag) Own(Condition.RunTag, TEXT("RunTag"));
					if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted && !Condition.GroupId.IsValid()) Errors.Add(TEXT("SpawnGroupCompleted requires GroupId."));
					if (Condition.Type == EDungeonStageCondition::SpawnGroupCompleted) Own(Condition.GroupId, TEXT("GroupId"));
				}
				if (bAlways && Index != Rule.Transitions.Num() - 1) Errors.Add(TEXT("Always branch shadows subsequent transitions."));
			}
		}
		if (Stage.TerminalOutcome == EDungeonTerminalOutcome::None && TransitionCount == 0) Errors.Add(Stage.StageId.ToString() + TEXT(": non-terminal has no transition."));
		if (Stage.TerminalOutcome != EDungeonTerminalOutcome::None && !Stage.EventRules.IsEmpty()) Errors.Add(Stage.StageId.ToString() + TEXT(": terminal rules would never execute."));
	}
	TSet<FGameplayTag> Visiting, Visited;
	TFunction<void(const FGameplayTag&)> Visit = [&](const FGameplayTag& Id)
	{
		if (Visiting.Contains(Id)) { Errors.Add(TEXT("Self/cyclic transition at ") + Id.ToString()); return; }
		if (Visited.Contains(Id)) return;
		Visiting.Add(Id);
		for (const FGameplayTag& Next : Edges.FindRef(Id)) Visit(Next);
		Visiting.Remove(Id); Visited.Add(Id);
	};
	Visit(StartStageId);
	const TSet<FGameplayTag> Reachable = Visited;
	for (const auto& Stage : Stages)
	{
		if (!Reachable.Contains(Stage.StageId)) Warnings.Add(TEXT("Unreachable stage: ") + Stage.StageId.ToString());
		Visit(Stage.StageId); // Cycles in disconnected authoring are also errors.
	}
	auto CanPrecede = [&](const FGameplayTag& Producer, const FGameplayTag& Consumer)
	{
		TSet<FGameplayTag> Seen; TArray<FGameplayTag> Pending{Producer};
		while (!Pending.IsEmpty())
		{
			const FGameplayTag Id = Pending.Pop(); if (Id == Consumer) return true;
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
			if (Rule.Event != EDungeonStageEvent::WaveCompleted && Rule.Event != EDungeonStageEvent::BossDefeated && Rule.Event != EDungeonStageEvent::MonsterCaptured) continue;
			const auto Producers = ProducersByGroup.FindRef(Rule.SourceId);
			bool bHasPath = false;
			for (const auto& Producer : Producers)
			{
				// A capture can come from either kind of group; a completion only from its own kind.
				if (Rule.Event != EDungeonStageEvent::MonsterCaptured && Producer.bBoss != (Rule.Event == EDungeonStageEvent::BossDefeated)) continue;
				if (Producer.Event == Rule.Event && Producer.Source == Rule.SourceId) continue; // Cannot spawn only after its own completion.
				bHasPath |= Reachable.Contains(Producer.Stage) && CanPrecede(Producer.Stage, Stage.StageId);
			}
			if (Reachable.Contains(Stage.StageId) && !bHasPath) Errors.Add(Stage.StageId.ToString() + TEXT(": no preceding spawn path for ") + Rule.SourceId.ToString());
		}
	}
	// Every successful run gets a grade: the last rule takes any time and needs no optional objective.
	for (const FDungeonGradeRule& Rule : GradeRules)
		if (Rule.Grade.IsNone()) Errors.Add(TEXT("A grade rule needs a Grade."));
	if (!GradeRules.IsEmpty() && (GradeRules.Last().MaxClearSeconds > 0.f || GradeRules.Last().RequiredOptionalObjectives > 0))
		Errors.Add(TEXT("The last grade rule must take every clear: MaxClearSeconds 0 and RequiredOptionalObjectives 0."));
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
	// Mermaid, because it renders in the places a design document already lives and stays readable as plain text. Names are written
	// without the dungeon's part of their tags, which the first line gives once.
	FString Text = FString::Printf(TEXT("flowchart TD\n  %%%% %s, start %s\n"), *DungeonId.ToString(), *Short(StartStageId, DungeonId));
	for (const FDungeonStageDefinition& Stage : Stages)
	{
		const FString Id = Node(Stage.StageId, DungeonId);
		if (Stage.TimeLimitSeconds > 0.f) Text += FString::Printf(TEXT("  %%%% %s time limit %.0fs\n"), *Id, Stage.TimeLimitSeconds);
		if (!Stage.SealedBarriers.IsEmpty())
			Text += FString::Printf(TEXT("  %%%% %s seals %s\n"), *Id, *FString::JoinBy(Stage.SealedBarriers, TEXT(", "), [this](const FGameplayTag& Barrier) { return Short(Barrier, DungeonId); }));
		if (Stage.TerminalOutcome != EDungeonTerminalOutcome::None)
		{
			Text += FString::Printf(TEXT("  %s[[\"%s %s\"]]\n"), *Id, *Short(Stage.StageId, DungeonId),
				Stage.TerminalOutcome == EDungeonTerminalOutcome::Success ? TEXT("성공") : TEXT("실패"));
		}
		for (const FDungeonStageEventRule& Rule : Stage.EventRules)
		{
			TArray<FString> Actions;
			for (const FDungeonStageAction& Action : Rule.Actions)
			{
				FString Entry = EnumName(StaticEnum<EDungeonStageAction>(), int64(Action.Type));
				if (Action.Type == EDungeonStageAction::SpawnGroup) Entry += TEXT(" ") + Short(Action.GroupId, DungeonId);
				if (Action.Type == EDungeonStageAction::SetRunTag || Action.Type == EDungeonStageAction::ClearRunTag) Entry += TEXT(" ") + Short(Action.RunTag, DungeonId);
				if (Action.Type == EDungeonStageAction::GrantReward) Entry += FString::Printf(TEXT(" %d x%d"), Action.RewardItemId, Action.RewardCount);
				Actions.Add(Entry);
			}
			const FString Event = EnumName(StaticEnum<EDungeonStageEvent>(), int64(Rule.Event)) +
				(Rule.SourceId.IsValid() ? TEXT(" ") + Short(Rule.SourceId, DungeonId) : FString());
			if (!Actions.IsEmpty()) Text += FString::Printf(TEXT("  %%%% %s on %s: %s\n"), *Id, *Event, *FString::Join(Actions, TEXT(", ")));
			for (const FDungeonStageTransition& Transition : Rule.Transitions)
			{
				TArray<FString> Conditions;
				for (const FDungeonStageCondition& Condition : Transition.Conditions) Conditions.Add(ConditionText(Condition, DungeonId));
				FString Label = Event + TEXT(" / ") + FString::Join(Conditions, TEXT(" and "));
				if (Transition.BranchId.IsValid()) Label += TEXT(" => ") + Short(Transition.BranchId, DungeonId);
				Text += FString::Printf(TEXT("  %s -->|\"%s\"| %s\n"), *Id, *Label, *Node(Transition.NextStageId, DungeonId));
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
