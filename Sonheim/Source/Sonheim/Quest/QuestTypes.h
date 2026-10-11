#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Interface.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "QuestTypes.generated.h"

/** What an objective asks of the player. */
UENUM(BlueprintType)
enum class EQuestGoal : uint8
{
	/** Defeat Required of species Id (0: any species). */
	Kill,
	/** Capture Required of species Id (0: any species). */
	Capture,
	/** Hold Required of item Id; counted from the inventory, never taken. */
	Possess,
	/** Collect Required of item Id from crafting stations. */
	Craft,
	/** Walk into region Target. */
	Reach,
	/** Win a run of the dungeon numbered Id (0: any dungeon). */
	ClearDungeon,
	/** Use quest target Target. */
	Talk,
	/** Hand Required of item Id to quest target Target, which takes what the player holds each time it is used. */
	Deliver,
};

USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EQuestGoal Goal = EQuestGoal::Kill;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Id = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Target;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Required = 1;
	/** The line the journal and the tracker show, such as 도로롱 포획. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
};

/** A stage of a quest: its objectives are open together, and all of them done opens the next step. */
USTRUCT(BlueprintType)
struct FQuestStep
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Summary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FQuestObjective> Objectives;
};

/** One quest, a row of DT_Quest. */
USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 QuestID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	/** The quest target that offers the quest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Giver;
	/** The quest target the finished quest is reported to; None finishes it as its last step is done. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Receiver;
	/** Quests that must be finished before this one is offered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> Prerequisites;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FQuestStep> Steps;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Exp = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FItemReward> Items;
};

UENUM(BlueprintType)
enum class EQuestState : uint8 { Active, ReadyToTurnIn, Completed };

/** One quest a player took. Replicated to its owner and kept in the save as it is. */
USTRUCT(BlueprintType)
struct FQuestRecord : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 QuestID = 0;
	UPROPERTY(BlueprintReadOnly) EQuestState State = EQuestState::Active;
	UPROPERTY(BlueprintReadOnly) int32 Step = 0;
	/** The count of each objective of the current step, in the step's order. */
	UPROPERTY(BlueprintReadOnly) TArray<int32> Progress;
	UPROPERTY(BlueprintReadOnly) bool bTracked = true;

	void PostReplicatedAdd(const struct FQuestList& List);
	void PostReplicatedChange(const struct FQuestList& List);
	void PreReplicatedRemove(const struct FQuestList& List);
};

USTRUCT()
struct FQuestList : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY() TArray<FQuestRecord> Items;
	UPROPERTY(NotReplicated, Transient) TObjectPtr<class UQuestComponent> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FQuestRecord, FQuestList>(Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FQuestList> : public TStructOpsTypeTraitsBase2<FQuestList>
{
	enum { WithNetDeltaSerializer = true };
};

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UQuestTarget : public UInterface
{
	GENERATED_BODY()
};

/** Something in the world quests name: a person who gives or receives quests, a site that takes deliveries. */
class SONHEIM_API IQuestTarget
{
	GENERATED_BODY()
public:
	virtual FName GetQuestTargetName() const = 0;
};
