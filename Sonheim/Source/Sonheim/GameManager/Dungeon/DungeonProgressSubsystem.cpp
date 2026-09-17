#include "DungeonProgressSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/LogMacro.h"

const TCHAR* UDungeonProgressSubsystem::SlotName = TEXT("DungeonProgress");

void UDungeonProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Save = Cast<UDungeonProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) Save = Cast<UDungeonProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UDungeonProgressSaveGame::StaticClass()));
	UE_LOG(SONHEIM, Log, TEXT("[DungeonProgress] loaded %d records"), Save ? Save->Records.Num() : 0);
}

const FDungeonClearRecord* UDungeonProgressSubsystem::FindRecord(FName DungeonId) const
{
	return Save ? Save->Records.FindByPredicate([DungeonId](const FDungeonClearRecord& Record) { return Record.DungeonId == DungeonId; }) : nullptr;
}

const FDungeonClearRecord& UDungeonProgressSubsystem::RecordRun(FName DungeonId, const FDungeonStageRuntimeState& State, bool bSuccess)
{
	static const FDungeonClearRecord Empty;
	if (!Save || DungeonId.IsNone()) return Empty;
	FDungeonClearRecord* Record = Save->Records.FindByPredicate([DungeonId](const FDungeonClearRecord& Value) { return Value.DungeonId == DungeonId; });
	if (!Record) Record = &Save->Records[Save->Records.AddDefaulted()];
	Record->DungeonId = DungeonId;
	Record->TotalDefeated += State.DefeatedCount;
	for (const FDungeonRunReward& Reward : State.Rewards)
	{
		if (FDungeonRunReward* Total = Record->TotalRewards.FindByPredicate([&Reward](const FDungeonRunReward& Value) { return Value.ItemId == Reward.ItemId; }))
			Total->Count += Reward.Count;
		else Record->TotalRewards.Add(Reward);
	}
	if (bSuccess)
	{
		++Record->ClearCount;
		Record->LastBranchId = State.SelectedBranchId;
		// The best time only counts finished runs, so a run that ended early never becomes the record.
		if (Record->BestSeconds <= 0.f || State.ElapsedSeconds < Record->BestSeconds) Record->BestSeconds = State.ElapsedSeconds;
	}
	else ++Record->FailCount;
	UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(SONHEIM, Log, TEXT("[DungeonProgress] %s clears=%d fails=%d best=%.1f"), *DungeonId.ToString(), Record->ClearCount, Record->FailCount, Record->BestSeconds);
	return *Record;
}
