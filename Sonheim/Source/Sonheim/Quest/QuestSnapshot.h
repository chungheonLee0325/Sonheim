#pragma once

#include "CoreMinimal.h"

#include "Sonheim/Quest/QuestData.h"

#include "QuestSnapshot.generated.h"

USTRUCT()
struct FQuestObjectiveProgressSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FName ObjectiveKey = NAME_None;

	UPROPERTY()
	int32 CurrentCount = 0;
};

USTRUCT()
struct FQuestInstanceSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 QuestID = 0;

	UPROPERTY()
	EQuestState State = EQuestState::None;

	UPROPERTY()
	int32 CurrentStepIndex = 0;

	UPROPERTY()
	bool bTracked = false;

	UPROPERTY()
	TArray<FQuestObjectiveProgressSnapshot> Objectives;
};

USTRUCT()
struct FQuestPlayerSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FQuestInstanceSnapshot> Quests;
};
