#include "MonsterDexComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InventoryComponent.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
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
}

FString UMonsterDexComponent::PlayerKey() const
{
	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	if (!PS || !PS->GetUniqueId().IsValid()) return FString();
	// The host's own player keeps one record on the host's machine. A remote player is kept under its net id, the same on every
	// visit through Steam; the null subsystem of an editor play session gives each session a new id, so there it lasts one session.
	const ULocalPlayer* Host = GetWorld()->GetGameInstance()->GetFirstGamePlayer();
	if (Host && Host->GetPreferredUniqueNetId() == PS->GetUniqueId()) return TEXT("Local");
	return PS->GetUniqueId().ToString();
}

void UMonsterDexComponent::LoadSaved()
{
	const UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>();
	const FString Key = PlayerKey();
	if (!GetOwner()->HasAuthority() || !Book || Key.IsEmpty()) return;
	Dex.Items = Book->LoadPlayer(Key);
	Dex.MarkArrayDirty();
	OnDexChanged.Broadcast();
}

const FMonsterDexEntry* UMonsterDexComponent::FindEntry(const int32 AreaObjectID) const
{
	return Dex.Items.FindByPredicate([AreaObjectID](const FMonsterDexEntry& E) { return E.AreaObjectID == AreaObjectID; });
}

void UMonsterDexComponent::NotifyKilled(const int32 AreaObjectID)
{
	Record(AreaObjectID, false);
}

void UMonsterDexComponent::NotifyCaptured(const int32 AreaObjectID)
{
	Record(AreaObjectID, true);
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

	const ASonheimPlayerState* PS = Cast<ASonheimPlayerState>(GetOwner());
	// A full inventory drops what does not fit at the player's feet, so nothing paid is lost.
	for (const FMonsterDexRewardItem& Item : Reward.Items)
	{
		if (PS && PS->m_InventoryComponent) PS->m_InventoryComponent->AddItem(Item.ItemId, Item.Count);
	}
	const ASonheimPlayer* Player = PS ? PS->GetPawn<ASonheimPlayer>() : nullptr;
	if (Reward.Exp > 0 && Player && Player->m_LevelComponent) Player->m_LevelComponent->AddExp(Reward.Exp);
}

void UMonsterDexComponent::Commit(FMonsterDexEntry& Entry)
{
	Dex.MarkItemDirty(Entry);
	OnDexChanged.Broadcast();
	if (UMonsterDexSubsystem* Book = GetWorld()->GetGameInstance()->GetSubsystem<UMonsterDexSubsystem>())
	{
		Book->StorePlayer(PlayerKey(), Dex.Items);
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
