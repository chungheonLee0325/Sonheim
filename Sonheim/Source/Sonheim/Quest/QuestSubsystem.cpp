#include "QuestSubsystem.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/LogMacro.h"

const TCHAR* UQuestSubsystem::SlotName = TEXT("Quests");

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const UDataTable* Table = QuestTable.LoadSynchronous())
	{
		Table->ForeachRow<FQuestData>(TEXT("Quest"), [this](const FName&, const FQuestData& Quest) { Quests.Add(Quest); });
		Quests.Sort([](const FQuestData& A, const FQuestData& B) { return A.QuestID < B.QuestID; });
	}
	Save = Cast<UQuestSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) Save = Cast<UQuestSaveGame>(UGameplayStatics::CreateSaveGameObject(UQuestSaveGame::StaticClass()));
	UE_LOG(SONHEIM, Log, TEXT("[Quest] %d quests, %d players saved"), Quests.Num(), Save ? Save->Players.Num() : 0);
}

const FQuestData* UQuestSubsystem::FindQuest(const int32 QuestID) const
{
	return Quests.FindByPredicate([QuestID](const FQuestData& Quest) { return Quest.QuestID == QuestID; });
}

TArray<FQuestRecord> UQuestSubsystem::LoadPlayer(const FString& PlayerKey) const
{
	const FQuestPlayerRecord* Record = Save ? Save->Players.FindByPredicate([&PlayerKey](const FQuestPlayerRecord& R) { return R.PlayerKey == PlayerKey; }) : nullptr;
	return Record ? Record->Quests : TArray<FQuestRecord>();
}

void UQuestSubsystem::StorePlayer(const FString& PlayerKey, const TArray<FQuestRecord>& Records)
{
	if (!Save || PlayerKey.IsEmpty()) return;
	FQuestPlayerRecord* Record = Save->Players.FindByPredicate([&PlayerKey](const FQuestPlayerRecord& R) { return R.PlayerKey == PlayerKey; });
	if (!Record) Record = &Save->Players.AddDefaulted_GetRef();
	Record->PlayerKey = PlayerKey;
	Record->Quests = Records;
	// ponytail: writes the whole slot on every change, as the monster book does; batch on a timer if progress gets frequent.
	if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0))
	{
		UE_LOG(SONHEIM, Error, TEXT("[Quest] could not write slot %s"), SlotName);
	}
}
