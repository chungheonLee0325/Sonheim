#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Quest/QuestTypes.h"
#include "QuestView.generated.h"

class UQuestComponent;

USTRUCT(BlueprintType)
struct FQuestObjectiveView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	UPROPERTY(BlueprintReadOnly) int32 Current = 0;
	UPROPERTY(BlueprintReadOnly) int32 Required = 1;
	UPROPERTY(BlueprintReadOnly) bool bDone = false;
};

/** Where a quest stands for the local player, as the screens show it; Offered is one a target gives but the player has not taken. */
UENUM(BlueprintType)
enum class EQuestViewState : uint8 { Offered, Active, ReadyToTurnIn, Completed };

USTRUCT(BlueprintType)
struct FQuestView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 QuestID = 0;
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Description;
	UPROPERTY(BlueprintReadOnly) EQuestViewState State = EQuestViewState::Offered;
	/** The step the player is at, from 0, and how many the quest has. */
	UPROPERTY(BlueprintReadOnly) int32 Step = 0;
	UPROPERTY(BlueprintReadOnly) int32 StepCount = 1;
	UPROPERTY(BlueprintReadOnly) FText StepSummary;
	/** The objectives of that step; a quest waiting to be reported shows a single line saying so. */
	UPROPERTY(BlueprintReadOnly) TArray<FQuestObjectiveView> Objectives;
	UPROPERTY(BlueprintReadOnly) TArray<FRewardChipView> Rewards;
	UPROPERTY(BlueprintReadOnly) bool bTracked = false;
};

namespace QuestView
{
	SONHEIM_API FQuestView Build(const UQuestComponent& Quests, int32 QuestID);
	/** The journal's list: quests under way first (waiting to be reported, then active), then finished ones, each in QuestID order. */
	SONHEIM_API TArray<FQuestView> Journal(const UQuestComponent& Quests);
	/** What a quest target has for the player: quests to report to it first, then quests it offers. */
	SONHEIM_API TArray<FQuestView> AtTarget(const UQuestComponent& Quests, FName Target);
	SONHEIM_API FText StateName(EQuestViewState State);
}
