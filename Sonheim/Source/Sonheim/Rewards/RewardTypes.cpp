#include "RewardTypes.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/StringTableIds.h"
#include "Engine/Texture2D.h"

TArray<FRewardChipView> SonheimReward::Describe(UGameInstance* Game, const TArray<FItemReward>& Items, const int32 Exp)
{
	TArray<FRewardChipView> Chips;
	USonheimGameInstance* Sonheim = Cast<USonheimGameInstance>(Game);
	for (const FItemReward& Item : Items)
	{
		const FItemData* Data = Sonheim ? Sonheim->GetDataItem(Item.ItemId) : nullptr;
		Chips.Add({Data ? Data->ItemIcon : nullptr, FText::Format(INVTEXT("×{0}"), Item.Count), Data ? Data->ItemName : FText::GetEmpty(), Item.ItemId, Item.Count});
	}
	if (Exp > 0)
	{
		static const TCHAR* StarIcon = TEXT("/Game/UI/Icons/T_IconExp.T_IconExp");
		Chips.Add({LoadObject<UTexture2D>(nullptr, StarIcon), FText::Format(LOCTABLE(SONHEIM_ST_NOTICE, "Reward.ExpFormat"), Exp), FText::GetEmpty()});
	}
	return Chips;
}

void SonheimReward::Grant(const ASonheimPlayerState& Player, const TArray<FItemReward>& Items, const int32 Exp)
{
	if (!Player.HasAuthority()) return;
	for (const FItemReward& Item : Items)
	{
		if (Player.m_InventoryComponent) Player.m_InventoryComponent->AddItem(Item.ItemId, Item.Count);
	}
	const ASonheimPlayer* Character = Player.GetPawn<ASonheimPlayer>();
	if (Exp > 0 && Character && Character->m_LevelComponent) Character->m_LevelComponent->AddExp(Exp);
}
