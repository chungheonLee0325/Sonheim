#include "DungeonObjectiveTracker.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"

bool UDungeonObjectiveTracker::RegisterGroup(FName Id, bool bBoss, const TArray<ABaseMonster*>& Monsters)
{
	if (Id.IsNone() || Groups.Contains(Id) || Monsters.IsEmpty()) return false;
	for (auto* Monster : Monsters) if (!IsValid(Monster) || !Monster->HasAuthority() || Monster->IsDie() || Monster->PartnerOwner) return false;
	auto& Group = Groups.Add(Id);
	Group.bBoss = bBoss;
	for (auto* Monster : Monsters)
	{
		Group.Monsters.Add(Monster);
		Monster->OnMonsterDeathConfirmed.AddUObject(this, &UDungeonObjectiveTracker::HandleDeath);
		Monster->OnMonsterBecamePartner.AddUObject(this, &UDungeonObjectiveTracker::HandlePartner);
		Monster->OnEndPlay.AddDynamic(this, &UDungeonObjectiveTracker::HandleEndPlay);
	}
	OnProgress.Broadcast(Id, 0, Group.Monsters.Num());
	return true;
}

void UDungeonObjectiveTracker::HandleDeath(ABaseMonster* Monster)
{
	if (!IsValid(Monster) || !Monster->HasAuthority() || !Monster->IsDie() || Monster->GetHP() > 0 || Monster->PartnerOwner) return;
	for (auto& Pair : Groups)
	{
		auto& Group = Pair.Value;
		if (!Group.Monsters.Contains(Monster) || Group.Defeated.Contains(Monster) || Group.Captured.Contains(Monster)) continue;
		Group.Defeated.Add(Monster);
		++TotalDefeated;
		const FName Id = Pair.Key;
		const int32 Count = Group.Resolved(), Required = Group.Monsters.Num();
		const bool bBoss = Group.bBoss;
		OnProgress.Broadcast(Id, Count, Required);
		if (Count == Required) OnCompleted.Broadcast(Id, bBoss);
		return; // callbacks may have ended/reset the run
	}
}

void UDungeonObjectiveTracker::HandlePartner(ABaseMonster* Monster)
{
	for (auto& Pair : Groups)
	{
		auto& Group = Pair.Value;
		if (!Group.Monsters.Contains(Monster) || Group.Defeated.Contains(Monster) || Group.Captured.Contains(Monster)) continue;
		// A captured monster leaves the fight for good, so it resolves its place in the group as a defeat does.
		Group.Captured.Add(Monster);
		++TotalCaptured;
		const FName Id = Pair.Key;
		const int32 Count = Group.Resolved(), Required = Group.Monsters.Num();
		const bool bBoss = Group.bBoss;
		OnProgress.Broadcast(Id, Count, Required);
		OnCaptured.Broadcast(Id);
		if (Count == Required) OnCompleted.Broadcast(Id, bBoss);
		return; // callbacks may have ended/reset the run
	}
}

void UDungeonObjectiveTracker::HandleEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	auto* Monster = Cast<ABaseMonster>(Actor);
	for (const auto& Pair : Groups)
		if (Pair.Value.Monsters.Contains(Monster) && !Pair.Value.Defeated.Contains(Monster) && !Pair.Value.Captured.Contains(Monster))
		{
			const FName Id = Pair.Key;
			OnInvalidated.Broadcast(Id); // Removal is never a kill, including world cleanup.
			return;
		}
}

bool UDungeonObjectiveTracker::IsComplete(FName Id) const
{
	const auto* Group = Groups.Find(Id);
	return Group && !Group->Monsters.IsEmpty() && Group->Resolved() == Group->Monsters.Num();
}

TArray<FDungeonGroupTally> UDungeonObjectiveTracker::GetTallies() const
{
	TArray<FDungeonGroupTally> Tallies;
	for (const auto& Pair : Groups) Tallies.Add({Pair.Key, Pair.Value.Monsters.Num(), Pair.Value.Defeated.Num(), Pair.Value.Captured.Num()});
	return Tallies;
}

void UDungeonObjectiveTracker::Reset(bool bDestroyMonsters)
{
	auto OldGroups = MoveTemp(Groups);
	Groups.Empty();
	TotalDefeated = 0;
	TotalCaptured = 0;
	for (auto& Pair : OldGroups)
		for (auto Weak : Pair.Value.Monsters)
			if (auto* Monster = Weak.Get())
			{
				Monster->OnMonsterDeathConfirmed.RemoveAll(this);
				Monster->OnMonsterBecamePartner.RemoveAll(this);
				Monster->OnEndPlay.RemoveDynamic(this, &UDungeonObjectiveTracker::HandleEndPlay);
				// Captured monsters belong to the player's inventory, not cleanup.
				if (bDestroyMonsters && !Monster->PartnerOwner) Monster->Destroy();
			}
}
