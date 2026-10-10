#include "RewardTypes.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"

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
