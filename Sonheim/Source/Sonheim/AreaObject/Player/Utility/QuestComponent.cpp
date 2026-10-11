#include "QuestComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "InventoryComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/Quest/QuestSubsystem.h"
#include "Sonheim/Utilities/StringTableIds.h"

void FQuestRecord::PostReplicatedAdd(const FQuestList& List)
{
	if (List.Owner) List.Owner->OnQuestsChanged.Broadcast();
}

void FQuestRecord::PostReplicatedChange(const FQuestList& List)
{
	if (List.Owner) List.Owner->OnQuestsChanged.Broadcast();
}

void FQuestRecord::PreReplicatedRemove(const FQuestList& List)
{
	if (List.Owner) List.Owner->OnQuestsChanged.Broadcast();
}

FSimpleMulticastDelegate UQuestComponent::OnAnyRecordChanged;

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UQuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UQuestComponent, Quests, COND_OwnerOnly);
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();
	Quests.Owner = this;
	ASonheimPlayerState* PS = Owner();
	if (PS && PS->HasAuthority())
	{
		PS->OnDeed.AddUObject(this, &UQuestComponent::HandleDeed);
		if (PS->m_InventoryComponent) PS->m_InventoryComponent->OnInventoryChanged.AddDynamic(this, &UQuestComponent::HandleInventory);
	}
}

ASonheimPlayerState* UQuestComponent::Owner() const
{
	return Cast<ASonheimPlayerState>(GetOwner());
}

const FQuestData* UQuestComponent::QuestOf(const int32 QuestID) const
{
	const UQuestSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>();
	return Book ? Book->FindQuest(QuestID) : nullptr;
}

FQuestRecord* UQuestComponent::Find(const int32 QuestID)
{
	return Quests.Items.FindByPredicate([QuestID](const FQuestRecord& R) { return R.QuestID == QuestID; });
}

const FQuestRecord* UQuestComponent::FindRecord(const int32 QuestID) const
{
	return Quests.Items.FindByPredicate([QuestID](const FQuestRecord& R) { return R.QuestID == QuestID; });
}

void UQuestComponent::LoadSaved()
{
	const UQuestSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>();
	const FString Key = Owner()->GetSaveKey();
	if (!GetOwner()->HasAuthority() || !Book || Key.IsEmpty()) return;
	Quests.Items = Book->LoadPlayer(Key);
	// A quest the table no longer has, or a step past its end, is dropped rather than kept half broken.
	Quests.Items.RemoveAll([this](const FQuestRecord& R)
	{
		const FQuestData* Quest = QuestOf(R.QuestID);
		return !Quest || (R.State == EQuestState::Active && !Quest->Steps.IsValidIndex(R.Step));
	});
	for (FQuestRecord& Record : Quests.Items)
	{
		const FQuestData& Quest = *QuestOf(Record.QuestID);
		if (Record.State == EQuestState::Active) Record.Progress.SetNum(Quest.Steps[Record.Step].Objectives.Num());
	}
	Quests.MarkArrayDirty();
	// Held items may have changed while the player was away.
	HandleInventory({});
	OnQuestsChanged.Broadcast();
	OnAnyRecordChanged.Broadcast();
}

bool UQuestComponent::CanAccept(const int32 QuestID) const
{
	const FQuestData* Quest = QuestOf(QuestID);
	if (!Quest || FindRecord(QuestID)) return false;
	for (const int32 Before : Quest->Prerequisites)
	{
		const FQuestRecord* Done = FindRecord(Before);
		if (!Done || Done->State != EQuestState::Completed) return false;
	}
	return true;
}

TArray<int32> UQuestComponent::GetOffers(const FName Giver) const
{
	TArray<int32> Offers;
	if (const UQuestSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
	{
		for (const FQuestData& Quest : Book->GetQuests())
		{
			if (Quest.Giver == Giver && CanAccept(Quest.QuestID)) Offers.Add(Quest.QuestID);
		}
	}
	return Offers;
}

TArray<int32> UQuestComponent::GetReports(const FName Receiver) const
{
	TArray<int32> Reports;
	for (const FQuestRecord& Record : Quests.Items)
	{
		const FQuestData* Quest = QuestOf(Record.QuestID);
		if (Record.State == EQuestState::ReadyToTurnIn && Quest && Quest->Receiver == Receiver) Reports.Add(Record.QuestID);
	}
	return Reports;
}

bool UQuestComponent::IsAt(AActor* Target, const FName Name) const
{
	// A quest with no giver or receiver needs no one to stand at.
	if (Name.IsNone()) return true;
	const IQuestTarget* Named = Cast<IQuestTarget>(Target);
	const APawn* Pawn = Owner()->GetPawn();
	return Named && Named->GetQuestTargetName() == Name && Pawn
		&& FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= ReachDistance;
}

void UQuestComponent::ServerAccept_Implementation(const int32 QuestID, AActor* Giver)
{
	// The server RPCs touch only the server's records; run in place on a client copy, as editor scripting does, they do nothing.
	const FQuestData* Quest = QuestOf(QuestID);
	if (!GetOwner()->HasAuthority() || !Quest || !CanAccept(QuestID) || !IsAt(Giver, Quest->Giver) || Quest->Steps.IsEmpty()) return;
	FQuestRecord& Record = Quests.Items.AddDefaulted_GetRef();
	Record.QuestID = QuestID;
	Record.Progress.SetNum(Quest->Steps[0].Objectives.Num());
	Notify(ENoticeTone::Information, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Accepted.Category"), Quest->Title, Quest->Steps[0].Summary);
	// Items the player already holds count at once.
	Settle(Record, *Quest);
	Changed(Record);
}

void UQuestComponent::ServerTurnIn_Implementation(const int32 QuestID, AActor* Receiver)
{
	FQuestRecord* Record = Find(QuestID);
	const FQuestData* Quest = QuestOf(QuestID);
	if (!GetOwner()->HasAuthority() || !Record || !Quest || Record->State != EQuestState::ReadyToTurnIn || !IsAt(Receiver, Quest->Receiver)) return;
	Finish(*Record, *Quest);
	Changed(*Record);
}

void UQuestComponent::ServerAbandon_Implementation(const int32 QuestID)
{
	const int32 Index = Quests.Items.IndexOfByPredicate([QuestID](const FQuestRecord& R) { return R.QuestID == QuestID && R.State != EQuestState::Completed; });
	if (!GetOwner()->HasAuthority() || Index == INDEX_NONE) return;
	Quests.Items.RemoveAt(Index);
	Quests.MarkArrayDirty();
	OnQuestsChanged.Broadcast();
	if (UQuestSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>()) Book->StorePlayer(Owner()->GetSaveKey(), Quests.Items);
}

void UQuestComponent::ServerTrack_Implementation(const int32 QuestID, const bool bTracked)
{
	FQuestRecord* Record = Find(QuestID);
	if (!GetOwner()->HasAuthority() || !Record || Record->bTracked == bTracked) return;
	Record->bTracked = bTracked;
	Changed(*Record);
}

bool UQuestComponent::Interact(AActor* Target)
{
	const IQuestTarget* Named = Cast<IQuestTarget>(Target);
	ASonheimPlayerState* PS = Owner();
	if (!GetOwner()->HasAuthority() || !Named) return false;
	const FName Name = Named->GetQuestTargetName();
	bool bAny = false;
	for (FQuestRecord& Record : Quests.Items)
	{
		const FQuestData* Quest = QuestOf(Record.QuestID);
		if (Record.State != EQuestState::Active || !Quest) continue;
		const TArray<FQuestObjective>& Open = Quest->Steps[Record.Step].Objectives;
		bool bMoved = false;
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			const FQuestObjective& Objective = Open[i];
			if (Objective.Target != Name || Record.Progress[i] >= Objective.Required) continue;
			if (Objective.Goal == EQuestGoal::Talk)
			{
				Record.Progress[i] = Objective.Required;
				bMoved = true;
			}
			else if (Objective.Goal == EQuestGoal::Deliver && PS->m_InventoryComponent)
			{
				const int32 Take = FMath::Min(PS->m_InventoryComponent->GetItemCount(Objective.Id), Objective.Required - Record.Progress[i]);
				if (Take > 0 && PS->m_InventoryComponent->RemoveItem(Objective.Id, Take))
				{
					Record.Progress[i] += Take;
					bMoved = true;
				}
			}
		}
		if (bMoved)
		{
			Settle(Record, *Quest);
			Changed(Record);
			bAny = true;
		}
	}
	return bAny;
}

void UQuestComponent::HandleDeed(const EPlayerDeed Deed, const int32 Id, const int32 Count, const FName Place)
{
	Advance([&](const FQuestObjective& Objective)
	{
		switch (Deed)
		{
		case EPlayerDeed::Kill: return Objective.Goal == EQuestGoal::Kill && (Objective.Id == 0 || Objective.Id == Id) ? Count : 0;
		case EPlayerDeed::Capture: return Objective.Goal == EQuestGoal::Capture && (Objective.Id == 0 || Objective.Id == Id) ? Count : 0;
		case EPlayerDeed::Craft: return Objective.Goal == EQuestGoal::Craft && Objective.Id == Id ? Count : 0;
		case EPlayerDeed::Reach: return Objective.Goal == EQuestGoal::Reach && Objective.Target == Place ? Objective.Required : 0;
		case EPlayerDeed::ClearDungeon: return Objective.Goal == EQuestGoal::ClearDungeon && (Objective.Id == 0 || Objective.Id == Id) ? Count : 0;
		default: return 0;
		}
	});
}

void UQuestComponent::HandleInventory(const TArray<FInventoryItem>&)
{
	// Settle recounts held items; only records whose count moves are sent.
	for (FQuestRecord& Record : Quests.Items)
	{
		const FQuestData* Quest = QuestOf(Record.QuestID);
		if (Record.State != EQuestState::Active || !Quest) continue;
		const TArray<int32> Before = Record.Progress;
		const int32 Step = Record.Step;
		Settle(Record, *Quest);
		if (Record.Progress != Before || Record.Step != Step || Record.State != EQuestState::Active) Changed(Record);
	}
}

void UQuestComponent::Advance(const TFunctionRef<int32(const FQuestObjective&)> Gain)
{
	if (!GetOwner()->HasAuthority()) return;
	for (FQuestRecord& Record : Quests.Items)
	{
		const FQuestData* Quest = QuestOf(Record.QuestID);
		if (Record.State != EQuestState::Active || !Quest) continue;
		const TArray<FQuestObjective>& Open = Quest->Steps[Record.Step].Objectives;
		bool bMoved = false;
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			const int32 Added = Gain(Open[i]);
			if (Added <= 0 || Record.Progress[i] >= Open[i].Required) continue;
			Record.Progress[i] = FMath::Min(Open[i].Required, Record.Progress[i] + Added);
			bMoved = true;
		}
		if (bMoved)
		{
			Settle(Record, *Quest);
			Changed(Record);
		}
	}
}

void UQuestComponent::Settle(FQuestRecord& Record, const FQuestData& Quest)
{
	const UInventoryComponent* Inventory = Owner()->m_InventoryComponent;
	while (Record.State == EQuestState::Active)
	{
		const TArray<FQuestObjective>& Open = Quest.Steps[Record.Step].Objectives;
		bool bDone = true;
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			if (Open[i].Goal == EQuestGoal::Possess && Inventory) Record.Progress[i] = FMath::Min(Open[i].Required, Inventory->GetItemCount(Open[i].Id));
			bDone &= Record.Progress[i] >= Open[i].Required;
		}
		if (!bDone) return;
		if (Quest.Steps.IsValidIndex(Record.Step + 1))
		{
			++Record.Step;
			Record.Progress.Init(0, Quest.Steps[Record.Step].Objectives.Num());
			Notify(ENoticeTone::Update, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Updated.Category"), Quest.Title, Quest.Steps[Record.Step].Summary);
		}
		else if (Quest.Receiver.IsNone())
		{
			Finish(Record, Quest);
		}
		else
		{
			Record.State = EQuestState::ReadyToTurnIn;
			Record.Progress.Reset();
			Notify(ENoticeTone::Highlight, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Ready.Category"), Quest.Title, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Ready.Detail"));
		}
	}
}

void UQuestComponent::Finish(FQuestRecord& Record, const FQuestData& Quest)
{
	// Marked before paying: a second report finds the quest finished and pays nothing.
	Record.State = EQuestState::Completed;
	Record.Progress.Reset();
	Record.bTracked = false;
	SonheimReward::Grant(*Owner(), Quest.Items, Quest.Exp);
	Notify(ENoticeTone::Success, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Completed.Category"), Quest.Title, LOCTABLE(SONHEIM_ST_QUEST, "Notice.Completed.Detail"));
}

void UQuestComponent::Changed(FQuestRecord& Record)
{
	Quests.MarkItemDirty(Record);
	OnQuestsChanged.Broadcast();
	OnAnyRecordChanged.Broadcast();
	if (UQuestSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>()) Book->StorePlayer(Owner()->GetSaveKey(), Quests.Items);
}

void UQuestComponent::DebugSet(const int32 QuestID, const EQuestState State)
{
	const FQuestData* Quest = QuestOf(QuestID);
	if (!GetOwner()->HasAuthority() || !Quest || Quest->Steps.IsEmpty()) return;
	FQuestRecord* Record = Find(QuestID);
	if (!Record)
	{
		Record = &Quests.Items.AddDefaulted_GetRef();
		Record->QuestID = QuestID;
	}
	Record->State = State;
	Record->Step = 0;
	Record->Progress.Init(0, State == EQuestState::Active ? Quest->Steps[0].Objectives.Num() : 0);
	if (State == EQuestState::Active) Settle(*Record, *Quest);
	Changed(*Record);
}

void UQuestComponent::Notify(const ENoticeTone Tone, const FText& Category, const FText& Title, const FText& Detail) const
{
	ASonheimPlayerController* Controller = Cast<ASonheimPlayerController>(Owner()->GetPlayerController());
	if (!Controller) return;
	FNoticeData Data;
	Data.Category = Category;
	Data.Title = Title;
	Data.Detail = Detail;
	Data.Symbol = INVTEXT("!");
	// The tone says which step of the quest it is, so it picks the icon too.
	Data.Icon = NoticeIcon(Tone == ENoticeTone::Update ? TEXT("T_IconQuestUpdate") : Tone == ENoticeTone::Highlight ? TEXT("T_IconQuestReady")
		: Tone == ENoticeTone::Success ? TEXT("T_IconQuestDone") : TEXT("T_IconQuestNew"));
	Data.Tone = Tone;
	Controller->PushNotice(ENoticeSlot::Feed, TEXT("Quest"), Data);
}
