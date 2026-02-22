#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "Sonheim/Quest/QuestData.h"
#include "Sonheim/Quest/QuestSnapshot.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"

#include "QuestComponent.generated.h"

class ASonheimPlayerState;
class UInventoryComponent;

struct FQuestOfferContext
{
	int32 NpcID = 0;
	float ExpiresAtTime = 0.f;
	FVector OfferLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> OfferActor = nullptr;
};

USTRUCT(BlueprintType)
struct FQuestObjectiveProgress
{
	GENERATED_BODY()

	UPROPERTY()
	FName ObjectiveKey = NAME_None;

	UPROPERTY()
	int32 CurrentCount = 0;
};

USTRUCT(BlueprintType)
struct FQuestInstance : public FFastArraySerializerItem
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
	TArray<FQuestObjectiveProgress> Objectives;

	int32 FindObjectiveIndex(FName Key) const;
	int32 GetObjectiveCount(FName Key) const;
	void SetObjectiveCount(FName Key, int32 NewCount);
};

USTRUCT()
struct FRepQuestList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FQuestInstance> Items;

	UPROPERTY(NotReplicated)
	class UQuestComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FQuestInstance, FRepQuestList>(Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FRepQuestList> : public TStructOpsTypeTraitsBase2<FRepQuestList>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestListChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_QuestList();

	// ===== Server RPC API =====
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest")
	void ServerAcceptQuest(int32 QuestID);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest")
	void ServerAbandonQuest(int32 QuestID);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest")
	void ServerSetTracked(int32 QuestID, bool bTracked);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest")
	void ServerTryTurnIn(int32 QuestID);

	// ===== Server-side event entrypoints =====
	void NotifyKilled(int32 VictimAreaObjectID);
	void NotifyItemDelta(int32 ItemID, int32 Delta, EItemChangeReason Reason);
	void RegisterQuestOffer(int32 QuestID, int32 NpcID, AActor* OfferActor, float OfferLifetimeSeconds = 15.f);
	void RegisterQuestTurnIn(int32 QuestID, int32 NpcID, AActor* OfferActor, float OfferLifetimeSeconds = 15.f);

	// ===== Travel persistence (session-only, server) =====
	FQuestPlayerSnapshot MakeSnapshot() const;
	void ApplySnapshot(const FQuestPlayerSnapshot& Snapshot);

	UPROPERTY(BlueprintAssignable, Category="Quest")
	FOnQuestListChanged OnQuestListChanged;

	UFUNCTION(BlueprintPure, Category="Quest")
	const TArray<FQuestInstance>& GetQuestInstances() const { return RepQuests.Items; }

private:
	ASonheimPlayerState* ResolvePlayerState() const;
	UInventoryComponent* ResolveInventory() const;

	FQuestInstance* FindQuest(int32 QuestID);
	const FQuestInstance* FindQuest(int32 QuestID) const;

	void InitialiseQuestInstanceFromDefinition(FQuestInstance& Inst, const FQuestData& Def);
	bool RecomputePossessObjectives(FQuestInstance& Inst, const FQuestData& Def);
	bool TryAdvanceSteps(FQuestInstance& Inst, const FQuestData& Def);
	bool IsStepComplete(const FQuestInstance& Inst, const FQuestData& Def, int32 StepIndex) const;
	bool HasAnyObjectiveInStep(const FQuestData& Def, int32 StepIndex) const;
	int32 FindNextStepIndex(const FQuestData& Def, int32 CurrentStepIndex) const;
	int32 FindFirstStepIndex(const FQuestData& Def) const;
	bool ReasonMatchesMask(EItemChangeReason Reason, int32 Mask) const;
	void GrantRewards(const FQuestData& Def);
	bool ArePrerequisitesSatisfied(const FQuestData& Def) const;
	bool ValidateAcceptRequest(int32 QuestID, const FQuestData& Def) const;
	bool ValidateTurnInRequest(int32 QuestID, const FQuestData& Def) const;
	bool ConsumeOfferIfValid(TMap<int32, FQuestOfferContext>& OfferMap, int32 QuestID, int32 ExpectedNpcID,
		float MaxDistance, bool bRequireNpcMatch) const;
	void RegisterOffer(TMap<int32, FQuestOfferContext>& OfferMap, int32 QuestID, int32 NpcID, AActor* OfferActor,
		float OfferLifetimeSeconds);
	void PruneExpiredOffers(TMap<int32, FQuestOfferContext>& OfferMap) const;
	bool CommitQuestIfDirty(FQuestInstance& Inst, bool bDirty, bool& bListDirty);
	void FlushQuestListIfDirty(bool bListDirty);
	AActor* ResolvePlayerActor() const;

	UFUNCTION()
	void OnInventoryChanged_Internal(const TArray<FInventoryItem>& Inventory);

	UFUNCTION()
	void OnItemDeltaDetailed_Internal(int32 ItemID, int32 Delta, EItemChangeReason Reason);

private:
	UPROPERTY(ReplicatedUsing=OnRep_QuestList)
	FRepQuestList RepQuests;

	mutable TMap<int32, FQuestOfferContext> AcceptOffers;
	mutable TMap<int32, FQuestOfferContext> TurnInOffers;
};
