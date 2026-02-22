#include "QuestComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Rewards/RewardService.h"

namespace
{
	constexpr float QuestOfferMaxDistance = 450.f;
}

int32 FQuestInstance::FindObjectiveIndex(FName Key) const
{
	for (int32 i = 0; i < Objectives.Num(); ++i)
	{
		if (Objectives[i].ObjectiveKey == Key)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 FQuestInstance::GetObjectiveCount(FName Key) const
{
	const int32 Idx = FindObjectiveIndex(Key);
	return (Idx != INDEX_NONE) ? Objectives[Idx].CurrentCount : 0;
}

void FQuestInstance::SetObjectiveCount(FName Key, int32 NewCount)
{
	const int32 Idx = FindObjectiveIndex(Key);
	if (Idx != INDEX_NONE)
	{
		Objectives[Idx].CurrentCount = NewCount;
	}
}

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UQuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UQuestComponent, RepQuests, COND_OwnerOnly);
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	RepQuests.Owner = this;

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (UInventoryComponent* Inv = ResolveInventory())
	{
		Inv->OnInventoryChanged.AddDynamic(this, &UQuestComponent::OnInventoryChanged_Internal);
		Inv->OnItemDeltaDetailed.AddDynamic(this, &UQuestComponent::OnItemDeltaDetailed_Internal);
	}
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UInventoryComponent* Inv = ResolveInventory())
		{
			Inv->OnInventoryChanged.RemoveDynamic(this, &UQuestComponent::OnInventoryChanged_Internal);
			Inv->OnItemDeltaDetailed.RemoveDynamic(this, &UQuestComponent::OnItemDeltaDetailed_Internal);
		}
	}

	AcceptOffers.Empty();
	TurnInOffers.Empty();
	OnQuestListChanged.Clear();

	Super::EndPlay(EndPlayReason);
}

void UQuestComponent::OnRep_QuestList()
{
	OnQuestListChanged.Broadcast();
}

ASonheimPlayerState* UQuestComponent::ResolvePlayerState() const
{
	return Cast<ASonheimPlayerState>(GetOwner());
}

UInventoryComponent* UQuestComponent::ResolveInventory() const
{
	if (ASonheimPlayerState* PS = ResolvePlayerState())
	{
		return PS->m_InventoryComponent;
	}
	return nullptr;
}

AActor* UQuestComponent::ResolvePlayerActor() const
{
	if (ASonheimPlayerState* PS = ResolvePlayerState())
	{
		if (AController* OwnerController = Cast<AController>(PS->GetOwner()))
		{
			if (APawn* Pawn = OwnerController->GetPawn())
			{
				return Pawn;
			}
		}

		if (APawn* Pawn = PS->GetPawn())
		{
			return Pawn;
		}
	}

	return nullptr;
}

FQuestInstance* UQuestComponent::FindQuest(int32 QuestID)
{
	for (FQuestInstance& Q : RepQuests.Items)
	{
		if (Q.QuestID == QuestID)
		{
			return &Q;
		}
	}
	return nullptr;
}

const FQuestInstance* UQuestComponent::FindQuest(int32 QuestID) const
{
	for (const FQuestInstance& Q : RepQuests.Items)
	{
		if (Q.QuestID == QuestID)
		{
			return &Q;
		}
	}
	return nullptr;
}

bool UQuestComponent::ReasonMatchesMask(EItemChangeReason Reason, int32 Mask) const
{
	if (Mask == 0)
	{
		// Default: count only true acquisition sources.
		switch (Reason)
		{
		case EItemChangeReason::LootPickup:
		case EItemChangeReason::CraftCollect:
		case EItemChangeReason::QuestReward:
			return true;
		default:
			return false;
		}
	}
	return (Mask & (int32)Reason) != 0;
}

void UQuestComponent::GrantRewards(const FQuestData& Def)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	ASonheimPlayerState* PS = ResolvePlayerState();
	if (!PS) return;

	const FRewardDef* Reward = FRewardService::ResolveRewardDef(GetWorld(), Def.RewardTableID, Def.Rewards);
	if (!Reward) return;

	const bool bHasExp = Reward->Exp > 0;
	const bool bHasItems = Reward->Items.Num() > 0;
	if (!bHasExp && !bHasItems)
	{
		return;
	}

	FRewardGrantOptions GrantOptions;
	GrantOptions.bGrantExpImmediately = true;
	GrantOptions.bGrantItemsToInventory = true;
	GrantOptions.InventoryItemReason = EItemChangeReason::QuestReward;
	GrantOptions.bDropItemsToWorld = false;
	FRewardService::GrantToPlayerState(PS, *Reward, GrantOptions);
}

void UQuestComponent::InitialiseQuestInstanceFromDefinition(FQuestInstance& Inst, const FQuestData& Def)
{
	Inst.QuestID = Def.QuestID;
	Inst.State = EQuestState::Active;
	const int32 FirstStep = FindFirstStepIndex(Def);
	Inst.CurrentStepIndex = (FirstStep != INDEX_NONE) ? FirstStep : 0;
	Inst.bTracked = false;
	Inst.Objectives.Empty();

	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		FQuestObjectiveProgress P;
		P.ObjectiveKey = Obj.ObjectiveKey;
		P.CurrentCount = 0;
		Inst.Objectives.Add(P);
	}

	RecomputePossessObjectives(Inst, Def);
	TryAdvanceSteps(Inst, Def);
}

bool UQuestComponent::RecomputePossessObjectives(FQuestInstance& Inst, const FQuestData& Def)
{
	UInventoryComponent* Inv = ResolveInventory();
	if (!Inv) return false;

	bool bDirty = false;

	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		if (Obj.StepIndex != Inst.CurrentStepIndex) continue;
		if (Obj.Type != EQuestObjectiveType::PossessItem) continue;
		if (Obj.ItemID <= 0) continue;

		const int32 Owned = Inv->GetItemCount(Obj.ItemID);
		const int32 NewCount = FMath::Clamp(Owned, 0, FMath::Max(0, Obj.RequiredCount));
		const int32 Cur = Inst.GetObjectiveCount(Obj.ObjectiveKey);
		if (Cur != NewCount)
		{
			Inst.SetObjectiveCount(Obj.ObjectiveKey, NewCount);
			bDirty = true;
		}
	}

	return bDirty;
}

bool UQuestComponent::HasAnyObjectiveInStep(const FQuestData& Def, int32 StepIndex) const
{
	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		if (Obj.StepIndex == StepIndex)
		{
			return true;
		}
	}
	return false;
}

bool UQuestComponent::IsStepComplete(const FQuestInstance& Inst, const FQuestData& Def, int32 StepIndex) const
{
	bool bHasAny = false;
	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		if (Obj.StepIndex != StepIndex) continue;
		bHasAny = true;
		const int32 Cur = Inst.GetObjectiveCount(Obj.ObjectiveKey);
		if (Cur < Obj.RequiredCount)
		{
			return false;
		}
	}
	return bHasAny;
}

int32 UQuestComponent::FindNextStepIndex(const FQuestData& Def, int32 CurrentStepIndex) const
{
	int32 Best = TNumericLimits<int32>::Max();
	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		if (Obj.StepIndex > CurrentStepIndex && Obj.StepIndex < Best)
		{
			Best = Obj.StepIndex;
		}
	}
	return (Best == TNumericLimits<int32>::Max()) ? INDEX_NONE : Best;
}

int32 UQuestComponent::FindFirstStepIndex(const FQuestData& Def) const
{
	int32 Best = TNumericLimits<int32>::Max();
	for (const FQuestObjectiveDef& Obj : Def.Objectives)
	{
		if (Obj.StepIndex < Best)
		{
			Best = Obj.StepIndex;
		}
	}
	return (Best == TNumericLimits<int32>::Max()) ? INDEX_NONE : Best;
}

bool UQuestComponent::TryAdvanceSteps(FQuestInstance& Inst, const FQuestData& Def)
{
	bool bDirty = false;

	while (Inst.State == EQuestState::Active)
	{
		if (!HasAnyObjectiveInStep(Def, Inst.CurrentStepIndex))
		{
			const int32 Next = FindNextStepIndex(Def, Inst.CurrentStepIndex);
			if (Next == INDEX_NONE)
			{
				GrantRewards(Def);
				Inst.State = EQuestState::Completed;
				bDirty = true;
				break;
			}

			Inst.CurrentStepIndex = Next;
			bDirty = true;
			bDirty |= RecomputePossessObjectives(Inst, Def);
			continue;
		}

		if (!IsStepComplete(Inst, Def, Inst.CurrentStepIndex))
		{
			break;
		}

		const int32 Next = FindNextStepIndex(Def, Inst.CurrentStepIndex);
		if (Next == INDEX_NONE)
		{
			GrantRewards(Def);
			Inst.State = EQuestState::Completed;
			bDirty = true;
			break;
		}

		Inst.CurrentStepIndex = Next;
		bDirty = true;
		bDirty |= RecomputePossessObjectives(Inst, Def);
	}

	return bDirty;
}

void UQuestComponent::PruneExpiredOffers(TMap<int32, FQuestOfferContext>& OfferMap) const
{
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	for (auto It = OfferMap.CreateIterator(); It; ++It)
	{
		if (It->Value.ExpiresAtTime > 0.f && Now > It->Value.ExpiresAtTime)
		{
			It.RemoveCurrent();
		}
	}
}

void UQuestComponent::RegisterOffer(TMap<int32, FQuestOfferContext>& OfferMap, int32 QuestID, int32 NpcID,
	AActor* OfferActor, float OfferLifetimeSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (QuestID <= 0) return;

	PruneExpiredOffers(OfferMap);

	FQuestOfferContext Context;
	Context.NpcID = NpcID;
	Context.OfferActor = OfferActor;
	Context.OfferLocation = OfferActor ? OfferActor->GetActorLocation() : FVector::ZeroVector;

	if (UWorld* World = GetWorld())
	{
		const float Lifetime = FMath::Max(0.f, OfferLifetimeSeconds);
		Context.ExpiresAtTime = (Lifetime > 0.f) ? (World->GetTimeSeconds() + Lifetime) : 0.f;
	}

	OfferMap.Add(QuestID, Context);
}

void UQuestComponent::RegisterQuestOffer(int32 QuestID, int32 NpcID, AActor* OfferActor, float OfferLifetimeSeconds)
{
	RegisterOffer(AcceptOffers, QuestID, NpcID, OfferActor, OfferLifetimeSeconds);
}

void UQuestComponent::RegisterQuestTurnIn(int32 QuestID, int32 NpcID, AActor* OfferActor, float OfferLifetimeSeconds)
{
	RegisterOffer(TurnInOffers, QuestID, NpcID, OfferActor, OfferLifetimeSeconds);
}

bool UQuestComponent::ConsumeOfferIfValid(TMap<int32, FQuestOfferContext>& OfferMap, int32 QuestID,
	int32 ExpectedNpcID, float MaxDistance, bool bRequireNpcMatch) const
{
	PruneExpiredOffers(OfferMap);

	FQuestOfferContext* Found = OfferMap.Find(QuestID);
	if (!Found)
	{
		return false;
	}

	if (bRequireNpcMatch && ExpectedNpcID > 0 && Found->NpcID != ExpectedNpcID)
	{
		return false;
	}

	AActor* PlayerActor = ResolvePlayerActor();
	if (!PlayerActor)
	{
		return false;
	}

	FVector OfferLoc = Found->OfferLocation;
	if (Found->OfferActor.IsValid())
	{
		OfferLoc = Found->OfferActor->GetActorLocation();
	}

	if (MaxDistance > 0.f)
	{
		const float DistSq = FVector::DistSquared(PlayerActor->GetActorLocation(), OfferLoc);
		if (DistSq > FMath::Square(MaxDistance))
		{
			return false;
		}
	}

	OfferMap.Remove(QuestID);
	return true;
}

bool UQuestComponent::ArePrerequisitesSatisfied(const FQuestData& Def) const
{
	for (const int32 PreReqQuestID : Def.PrereqQuestIDs)
	{
		if (PreReqQuestID <= 0) continue;

		const FQuestInstance* PreReq = FindQuest(PreReqQuestID);
		if (!PreReq || PreReq->State != EQuestState::Completed)
		{
			return false;
		}
	}

	return true;
}

bool UQuestComponent::ValidateAcceptRequest(int32 QuestID, const FQuestData& Def) const
{
	if (QuestID <= 0)
	{
		return false;
	}

	if (FindQuest(QuestID))
	{
		return false;
	}

	if (!ArePrerequisitesSatisfied(Def))
	{
		return false;
	}

	const bool bNpcMatchRequired = (Def.StartNpcID > 0);
	return ConsumeOfferIfValid(AcceptOffers, QuestID, Def.StartNpcID, QuestOfferMaxDistance, bNpcMatchRequired);
}

bool UQuestComponent::ValidateTurnInRequest(int32 QuestID, const FQuestData& Def) const
{
	const bool bNpcMatchRequired = (Def.EndNpcID > 0);
	return ConsumeOfferIfValid(TurnInOffers, QuestID, Def.EndNpcID, QuestOfferMaxDistance, bNpcMatchRequired);
}

bool UQuestComponent::CommitQuestIfDirty(FQuestInstance& Inst, bool bDirty, bool& bListDirty)
{
	if (!bDirty)
	{
		return false;
	}

	RepQuests.MarkItemDirty(Inst);
	bListDirty = true;
	return true;
}

void UQuestComponent::FlushQuestListIfDirty(bool bListDirty)
{
	if (!bListDirty)
	{
		return;
	}

	if (GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
	OnRep_QuestList();
}

void UQuestComponent::ServerAcceptQuest_Implementation(int32 QuestID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld());
	if (!GI) return;

	FQuestData* Def = GI->GetDataQuest(QuestID);
	if (!Def) return;

	if (!ValidateAcceptRequest(QuestID, *Def))
	{
		return;
	}

	FQuestInstance& NewInst = RepQuests.Items.AddDefaulted_GetRef();
	InitialiseQuestInstanceFromDefinition(NewInst, *Def);
	RepQuests.MarkItemDirty(NewInst);
	FlushQuestListIfDirty(true);

	if (ASonheimPlayerState* PS = ResolvePlayerState())
	{
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(PS->GetOwner()))
		{
			PC->Client_ShowQuestAcceptedToast(QuestID);
		}
	}
}

void UQuestComponent::ServerAbandonQuest_Implementation(int32 QuestID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (int32 i = 0; i < RepQuests.Items.Num(); ++i)
	{
		if (RepQuests.Items[i].QuestID == QuestID)
		{
			if (RepQuests.Items[i].State != EQuestState::Active)
			{
				return;
			}

			RepQuests.Items.RemoveAt(i);
			RepQuests.MarkArrayDirty();
			FlushQuestListIfDirty(true);
			return;
		}
	}
}

void UQuestComponent::ServerSetTracked_Implementation(int32 QuestID, bool bInTracked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (FQuestInstance* Inst = FindQuest(QuestID))
	{
		if (Inst->bTracked != bInTracked)
		{
			Inst->bTracked = bInTracked;
			RepQuests.MarkItemDirty(*Inst);
			FlushQuestListIfDirty(true);
		}
	}
}

void UQuestComponent::ServerTryTurnIn_Implementation(int32 QuestID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	FQuestInstance* Inst = FindQuest(QuestID);
	if (!Inst || Inst->State != EQuestState::Active) return;

	USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld());
	if (!GI) return;

	FQuestData* Def = GI->GetDataQuest(QuestID);
	if (!Def) return;

	if (!ValidateTurnInRequest(QuestID, *Def))
	{
		return;
	}

	UInventoryComponent* Inv = ResolveInventory();
	if (!Inv) return;

	// Validate all turn-in objectives in current step first.
	for (const FQuestObjectiveDef& Obj : Def->Objectives)
	{
		if (Obj.StepIndex != Inst->CurrentStepIndex) continue;
		if (Obj.Type != EQuestObjectiveType::TurnInItem) continue;
		if (Obj.ItemID <= 0) return;

		const int32 Cur = Inst->GetObjectiveCount(Obj.ObjectiveKey);
		const int32 Need = FMath::Max(0, Obj.RequiredCount - Cur);
		if (Need > 0 && !Inv->HasItem(Obj.ItemID, Need))
		{
			return;
		}
	}

	bool bDirty = false;

	// Consume and progress.
	for (const FQuestObjectiveDef& Obj : Def->Objectives)
	{
		if (Obj.StepIndex != Inst->CurrentStepIndex) continue;
		if (Obj.Type != EQuestObjectiveType::TurnInItem) continue;
		if (Obj.ItemID <= 0) continue;

		const int32 Cur = Inst->GetObjectiveCount(Obj.ObjectiveKey);
		const int32 Need = FMath::Max(0, Obj.RequiredCount - Cur);
		if (Need <= 0) continue;

		if (!Inv->RemoveItem(Obj.ItemID, Need))
		{
			return;
		}
		Inst->SetObjectiveCount(Obj.ObjectiveKey, Cur + Need);
		bDirty = true;
	}

	bDirty |= TryAdvanceSteps(*Inst, *Def);

	bool bListDirty = false;
	CommitQuestIfDirty(*Inst, bDirty, bListDirty);
	FlushQuestListIfDirty(bListDirty);
}

void UQuestComponent::NotifyKilled(int32 VictimAreaObjectID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (VictimAreaObjectID <= 0) return;

	USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld());
	if (!GI) return;

	bool bListDirty = false;

	for (FQuestInstance& Inst : RepQuests.Items)
	{
		if (Inst.State != EQuestState::Active) continue;
		FQuestData* Def = GI->GetDataQuest(Inst.QuestID);
		if (!Def) continue;

		bool bDirty = false;
		for (const FQuestObjectiveDef& Obj : Def->Objectives)
		{
			if (Obj.StepIndex != Inst.CurrentStepIndex) continue;
			if (Obj.Type != EQuestObjectiveType::Kill) continue;
			if (Obj.TargetAreaObjectID != 0 && Obj.TargetAreaObjectID != VictimAreaObjectID) continue;

			const int32 Cur = Inst.GetObjectiveCount(Obj.ObjectiveKey);
			const int32 NewCount = FMath::Min(Cur + 1, Obj.RequiredCount);
			if (NewCount != Cur)
			{
				Inst.SetObjectiveCount(Obj.ObjectiveKey, NewCount);
				bDirty = true;
			}
		}

		if (bDirty)
		{
			bDirty |= TryAdvanceSteps(Inst, *Def);
			CommitQuestIfDirty(Inst, bDirty, bListDirty);
		}
	}

	FlushQuestListIfDirty(bListDirty);
}

void UQuestComponent::NotifyItemDelta(int32 ItemID, int32 Delta, EItemChangeReason Reason)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (ItemID <= 0 || Delta <= 0) return;

	USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld());
	if (!GI) return;

	bool bListDirty = false;

	for (FQuestInstance& Inst : RepQuests.Items)
	{
		if (Inst.State != EQuestState::Active) continue;
		FQuestData* Def = GI->GetDataQuest(Inst.QuestID);
		if (!Def) continue;

		bool bDirty = false;
		for (const FQuestObjectiveDef& Obj : Def->Objectives)
		{
			if (Obj.StepIndex != Inst.CurrentStepIndex) continue;
			if (Obj.ItemID != ItemID) continue;

			const bool bAcquire = (Obj.Type == EQuestObjectiveType::AcquireItem);
			const bool bCraft = (Obj.Type == EQuestObjectiveType::CraftItem);
			if (!bAcquire && !bCraft) continue;

			if (!ReasonMatchesMask(Reason, Obj.ReasonMask)) continue;

			const int32 Cur = Inst.GetObjectiveCount(Obj.ObjectiveKey);
			const int32 NewCount = FMath::Min(Cur + Delta, Obj.RequiredCount);
			if (NewCount != Cur)
			{
				Inst.SetObjectiveCount(Obj.ObjectiveKey, NewCount);
				bDirty = true;
			}
		}

		if (bDirty)
		{
			bDirty |= TryAdvanceSteps(Inst, *Def);
			CommitQuestIfDirty(Inst, bDirty, bListDirty);
		}
	}

	FlushQuestListIfDirty(bListDirty);
}

FQuestPlayerSnapshot UQuestComponent::MakeSnapshot() const
{
	FQuestPlayerSnapshot Out;
	Out.Quests.Reserve(RepQuests.Items.Num());

	for (const FQuestInstance& Inst : RepQuests.Items)
	{
		FQuestInstanceSnapshot S;
		S.QuestID = Inst.QuestID;
		S.State = Inst.State;
		S.CurrentStepIndex = Inst.CurrentStepIndex;
		S.bTracked = Inst.bTracked;
		S.Objectives.Reserve(Inst.Objectives.Num());
		for (const FQuestObjectiveProgress& P : Inst.Objectives)
		{
			FQuestObjectiveProgressSnapshot PS;
			PS.ObjectiveKey = P.ObjectiveKey;
			PS.CurrentCount = P.CurrentCount;
			S.Objectives.Add(PS);
		}
		Out.Quests.Add(S);
	}

	return Out;
}

void UQuestComponent::ApplySnapshot(const FQuestPlayerSnapshot& Snapshot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bListDirty = false;

	if (RepQuests.Items.Num() > 0)
	{
		RepQuests.Items.Empty();
		RepQuests.MarkArrayDirty();
		bListDirty = true;
	}

	for (const FQuestInstanceSnapshot& S : Snapshot.Quests)
	{
		FQuestInstance& Inst = RepQuests.Items.AddDefaulted_GetRef();
		Inst.QuestID = S.QuestID;
		Inst.State = (S.State == EQuestState::Completed) ? EQuestState::Completed : EQuestState::Active;
		Inst.CurrentStepIndex = S.CurrentStepIndex;
		Inst.bTracked = S.bTracked;
		Inst.Objectives.Empty();
		for (const FQuestObjectiveProgressSnapshot& PS : S.Objectives)
		{
			FQuestObjectiveProgress P;
			P.ObjectiveKey = PS.ObjectiveKey;
			P.CurrentCount = PS.CurrentCount;
			Inst.Objectives.Add(P);
		}
		RepQuests.MarkItemDirty(Inst);
		bListDirty = true;
	}

	// Recompute possess objectives for the currently active step (inventory snapshot may differ).
	if (USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld()))
	{
		for (FQuestInstance& Inst : RepQuests.Items)
		{
			if (Inst.State != EQuestState::Active) continue;
			if (FQuestData* Def = GI->GetDataQuest(Inst.QuestID))
			{
				bool bDirty = false;
				bDirty |= RecomputePossessObjectives(Inst, *Def);
				bDirty |= TryAdvanceSteps(Inst, *Def);
				CommitQuestIfDirty(Inst, bDirty, bListDirty);
			}
		}
	}

	FlushQuestListIfDirty(bListDirty);
}

void UQuestComponent::OnInventoryChanged_Internal(const TArray<FInventoryItem>& Inventory)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	(void)Inventory;

	USonheimGameInstance* GI = USonheimGameInstance::Get(GetWorld());
	if (!GI) return;

	bool bListDirty = false;

	for (FQuestInstance& Inst : RepQuests.Items)
	{
		if (Inst.State != EQuestState::Active) continue;
		if (FQuestData* Def = GI->GetDataQuest(Inst.QuestID))
		{
			bool bDirty = false;
			bDirty |= RecomputePossessObjectives(Inst, *Def);
			bDirty |= TryAdvanceSteps(Inst, *Def);
			CommitQuestIfDirty(Inst, bDirty, bListDirty);
		}
	}

	FlushQuestListIfDirty(bListDirty);
}

void UQuestComponent::OnItemDeltaDetailed_Internal(int32 ItemID, int32 Delta, EItemChangeReason Reason)
{
	NotifyItemDelta(ItemID, Delta, Reason);
}
