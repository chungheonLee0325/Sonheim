#include "MonsterDexView.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Sonheim/AreaObject/Player/Utility/MonsterDexComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/MonsterDex/MonsterDexSubsystem.h"
#include "Sonheim/Utilities/StringTableIds.h"

namespace
{
	const TCHAR* StarIcon = TEXT("/Game/CuratedDungeon/UI/Icons/T_Icon_Star.T_Icon_Star");

	FMonsterDexTierView TierView(const FMonsterDexRewardTier& Tier, const int32 Index, const FMonsterDexEntry* Entry, USonheimGameInstance* Game)
	{
		FMonsterDexTierView View;
		View.Tier = Index;
		// A tier asks for defeats or for captures; the book's tiers never ask for both.
		View.bCapture = Tier.RequiredCaptures > 0;
		View.Required = FMath::Max(1, View.bCapture ? Tier.RequiredCaptures : Tier.RequiredKills);
		View.Current = FMath::Min(View.Required, Entry ? (View.bCapture ? Entry->CaptureCount : Entry->KillCount) : 0);
		View.Condition = FText::Format(View.bCapture ? LOCTABLE(SONHEIM_ST_DEX, "Tier.CaptureFormat") : LOCTABLE(SONHEIM_ST_DEX, "Tier.KillFormat"), View.Required);
		View.State = Entry && Entry->IsClaimed(Index) ? EMonsterDexTierState::Claimed
			: Entry && Entry->Meets(Tier) ? EMonsterDexTierState::Ready : EMonsterDexTierState::InProgress;
		for (const FItemReward& Item : Tier.Items)
		{
			const FItemData* Data = Game ? Game->GetDataItem(Item.ItemId) : nullptr;
			View.Rewards.Add({Data ? Data->ItemIcon : nullptr, FText::Format(INVTEXT("×{0}"), Item.Count), Data ? Data->ItemName : FText::GetEmpty()});
		}
		if (Tier.Exp > 0)
		{
			View.Rewards.Add({LoadObject<UTexture2D>(nullptr, StarIcon), FText::Format(LOCTABLE(SONHEIM_ST_DEX, "Reward.ExpFormat"), Tier.Exp), FText::GetEmpty()});
		}
		return View;
	}
}

FText MonsterDexView::NumberText(const int32 Number)
{
	FNumberFormattingOptions Digits;
	Digits.MinimumIntegralDigits = 3;
	Digits.UseGrouping = false;
	return FText::Format(INVTEXT("No.{0}"), FText::AsNumber(Number, &Digits));
}

FText MonsterDexView::ElementName(const EElementalAttribute Element)
{
	switch (Element)
	{
	case EElementalAttribute::Grass: return LOCTABLE(SONHEIM_ST_DEX, "Element.Grass");
	case EElementalAttribute::Fire: return LOCTABLE(SONHEIM_ST_DEX, "Element.Fire");
	case EElementalAttribute::Water: return LOCTABLE(SONHEIM_ST_DEX, "Element.Water");
	case EElementalAttribute::Electric: return LOCTABLE(SONHEIM_ST_DEX, "Element.Electric");
	case EElementalAttribute::Ground: return LOCTABLE(SONHEIM_ST_DEX, "Element.Ground");
	case EElementalAttribute::Ice: return LOCTABLE(SONHEIM_ST_DEX, "Element.Ice");
	case EElementalAttribute::Dragon: return LOCTABLE(SONHEIM_ST_DEX, "Element.Dragon");
	case EElementalAttribute::Dark: return LOCTABLE(SONHEIM_ST_DEX, "Element.Dark");
	case EElementalAttribute::Neutral: return LOCTABLE(SONHEIM_ST_DEX, "Element.Neutral");
	default: return FText::GetEmpty();
	}
}

FMonsterDexBookView MonsterDexView::Build(const UMonsterDexComponent& Dex)
{
	FMonsterDexBookView Book;
	UGameInstance* Instance = Dex.GetWorld() ? Dex.GetWorld()->GetGameInstance() : nullptr;
	const UMonsterDexSubsystem* Pages = Instance ? Instance->GetSubsystem<UMonsterDexSubsystem>() : nullptr;
	USonheimGameInstance* Game = Cast<USonheimGameInstance>(Instance);
	if (!Pages) return Book;

	for (const FMonsterDexData& Page : Pages->GetPages())
	{
		const FMonsterDexEntry* Entry = Dex.FindEntry(Page.AreaObjectID);
		const FAreaObjectData* Species = Game ? Game->GetDataAreaObject(Page.AreaObjectID) : nullptr;
		FMonsterDexPageView& View = Book.Pages.AddDefaulted_GetRef();
		View.AreaObjectID = Page.AreaObjectID;
		View.Number = Page.Number;
		View.bDiscovered = Entry != nullptr;
		View.bCaptured = Entry && Entry->CaptureCount > 0;
		// The portrait shows on an undiscovered page too, where the cell draws it as a silhouette.
		View.Portrait = Species ? Species->AreaObjectIcon : nullptr;
		View.DisplayClass = Page.DisplayClass;
		View.Kills = Entry ? Entry->KillCount : 0;
		View.Captures = Entry ? Entry->CaptureCount : 0;
		if (View.bDiscovered)
		{
			View.Name = Page.DisplayName;
			View.Description = Page.Description;
			if (Species) View.Elements = Species->DefenceElementalAttributes.Array();
		}
		else
		{
			View.Name = INVTEXT("???");
			View.Description = LOCTABLE(SONHEIM_ST_DEX, "Page.Unknown.Description");
		}
		for (int32 Tier = 0; Tier < Page.RewardTiers.Num(); ++Tier)
		{
			View.Tiers.Add(TierView(Page.RewardTiers[Tier], Tier, Entry, Game));
			View.bRewardReady |= View.Tiers.Last().State == EMonsterDexTierState::Ready;
		}
		Book.Discovered += View.bDiscovered;
		Book.Captured += View.bCaptured;
	}
	return Book;
}
