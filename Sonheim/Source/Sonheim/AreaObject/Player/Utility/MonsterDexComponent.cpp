#include "MonsterDexComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/MonsterDex/MonsterDexSubsystem.h"
#include "Sonheim/Utilities/StringTableIds.h"

void FMonsterDexEntry::PostReplicatedAdd(const FMonsterDexList& List)
{
	if (List.Owner) List.Owner->OnDexChanged.Broadcast();
}

void FMonsterDexEntry::PostReplicatedChange(const FMonsterDexList& List)
{
	if (List.Owner) List.Owner->OnDexChanged.Broadcast();
}

UMonsterDexComponent::UMonsterDexComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMonsterDexComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UMonsterDexComponent, Dex, COND_OwnerOnly);
}

void UMonsterDexComponent::BeginPlay()
{
	Super::BeginPlay();
	Dex.Owner = this;
	ASonheimPlayerState* PS = Cast<ASonheimPlayerState>(GetOwner());
	if (PS && PS->HasAuthority()) PS->OnDeed.AddUObject(this, &UMonsterDexComponent::HandleDeed);
}

void UMonsterDexComponent::HandleDeed(const EPlayerDeed Deed, const int32 Id, int32, FName)
{
	if (Deed == EPlayerDeed::Kill || Deed == EPlayerDeed::Capture) Record(Id, Deed == EPlayerDeed::Capture);
}

void UMonsterDexComponent::LoadSaved()
{
	const UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>();
	const FString Key = Cast<ASonheimPlayerState>(GetOwner())->GetSaveKey();
	if (!GetOwner()->HasAuthority() || !Book || Key.IsEmpty()) return;
	Dex.Items = Book->LoadPlayer(Key);
	Dex.MarkArrayDirty();
	OnDexChanged.Broadcast();
}

const FMonsterDexEntry* UMonsterDexComponent::FindEntry(const int32 AreaObjectID) const
{
	return Dex.Items.FindByPredicate([AreaObjectID](const FMonsterDexEntry& E) { return E.AreaObjectID == AreaObjectID; });
}

void UMonsterDexComponent::Record(const int32 AreaObjectID, const bool bCaptured)
{
	const UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>();
	const FMonsterDexData* Page = Book ? Book->FindPage(AreaObjectID) : nullptr;
	// Species without a page, such as bosses, are not in the book.
	if (!GetOwner()->HasAuthority() || !Page) return;

	FMonsterDexEntry* Entry = Dex.Items.FindByPredicate([AreaObjectID](const FMonsterDexEntry& E) { return E.AreaObjectID == AreaObjectID; });
	const bool bNew = !Entry;
	if (bNew)
	{
		Entry = &Dex.Items.AddDefaulted_GetRef();
		Entry->AreaObjectID = AreaObjectID;
	}
	TArray<bool> WasReady;
	for (int32 Tier = 0; Tier < Page->RewardTiers.Num(); ++Tier) WasReady.Add(Entry->Meets(Page->RewardTiers[Tier]));
	++(bCaptured ? Entry->CaptureCount : Entry->KillCount);

	if (bNew)
	{
		Notify(FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Notice.Registered.Title"), Page->DisplayName),
		       FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Notice.Registered.Detail"), FText::AsNumber(Page->Number)));
	}
	for (int32 Tier = 0; Tier < Page->RewardTiers.Num(); ++Tier)
	{
		if (!WasReady[Tier] && Entry->Meets(Page->RewardTiers[Tier]) && !Entry->IsClaimed(Tier))
		{
			Notify(FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Notice.RewardReady.Title"), Page->DisplayName),
			       LOCTABLE(SONHEIM_ST_DEX, "Notice.RewardReady.Detail"));
		}
	}
	Commit(*Entry);
}

bool UMonsterDexComponent::CanClaimReward(const int32 AreaObjectID, const int32 Tier) const
{
	const UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>();
	const FMonsterDexData* Page = Book ? Book->FindPage(AreaObjectID) : nullptr;
	const FMonsterDexEntry* Entry = FindEntry(AreaObjectID);
	return Page && Entry && Page->RewardTiers.IsValidIndex(Tier) && Tier < 32
		&& !Entry->IsClaimed(Tier) && Entry->Meets(Page->RewardTiers[Tier]);
}

void UMonsterDexComponent::ServerClaimReward_Implementation(const int32 AreaObjectID, const int32 Tier)
{
	// The claimed bit is set before anything is paid, so a repeated request finds it set and pays nothing.
	if (!GetOwner()->HasAuthority() || !CanClaimReward(AreaObjectID, Tier)) return;
	const FMonsterDexRewardTier& Reward = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>()->FindPage(AreaObjectID)->RewardTiers[Tier];
	FMonsterDexEntry* Entry = Dex.Items.FindByPredicate([AreaObjectID](const FMonsterDexEntry& E) { return E.AreaObjectID == AreaObjectID; });
	Entry->ClaimedTiers |= 1 << Tier;
	Commit(*Entry);

	SonheimReward::Grant(*Cast<ASonheimPlayerState>(GetOwner()), Reward.Items, Reward.Exp);
}

void UMonsterDexComponent::Commit(FMonsterDexEntry& Entry)
{
	Dex.MarkItemDirty(Entry);
	OnDexChanged.Broadcast();
	if (UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>())
	{
		Book->StorePlayer(Cast<ASonheimPlayerState>(GetOwner())->GetSaveKey(), Dex.Items);
	}
}

void UMonsterDexComponent::Notify(const FText& Title, const FText& Detail) const
{
	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	ASonheimPlayerController* Controller = PS ? Cast<ASonheimPlayerController>(PS->GetPlayerController()) : nullptr;
	if (!Controller) return;
	FNoticeData Data;
	Data.Category = LOCTABLE(SONHEIM_ST_DEX, "Notice.Category");
	Data.Title = Title;
	Data.Detail = Detail;
	Data.Symbol = INVTEXT("◆");
	Data.Tone = ENoticeTone::Highlight;
	Controller->PushNotice(ENoticeSlot::Banner, TEXT("Dex"), Data);
}
