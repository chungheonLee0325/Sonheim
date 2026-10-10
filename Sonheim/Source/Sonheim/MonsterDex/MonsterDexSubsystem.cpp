#include "MonsterDexSubsystem.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/LogMacro.h"

const TCHAR* UMonsterDexSubsystem::SlotName = TEXT("MonsterDex");

void UMonsterDexSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const UDataTable* Table = PageTable.LoadSynchronous())
	{
		Table->ForeachRow<FMonsterDexData>(TEXT("MonsterDex"), [this](const FName&, const FMonsterDexData& Page) { Pages.Add(Page); });
		Pages.Sort([](const FMonsterDexData& A, const FMonsterDexData& B) { return A.Number < B.Number; });
	}
	Save = Cast<UMonsterDexSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save) Save = Cast<UMonsterDexSaveGame>(UGameplayStatics::CreateSaveGameObject(UMonsterDexSaveGame::StaticClass()));
	UE_LOG(SONHEIM, Log, TEXT("[MonsterDex] %d pages, %d players saved"), Pages.Num(), Save ? Save->Players.Num() : 0);
}

const FMonsterDexData* UMonsterDexSubsystem::FindPage(const int32 AreaObjectID) const
{
	return Pages.FindByPredicate([AreaObjectID](const FMonsterDexData& Page) { return Page.AreaObjectID == AreaObjectID; });
}

TArray<FMonsterDexEntry> UMonsterDexSubsystem::LoadPlayer(const FString& PlayerKey) const
{
	const FMonsterDexPlayerRecord* Record = Save ? Save->Players.FindByPredicate([&PlayerKey](const FMonsterDexPlayerRecord& R) { return R.PlayerKey == PlayerKey; }) : nullptr;
	return Record ? Record->Entries : TArray<FMonsterDexEntry>();
}

void UMonsterDexSubsystem::StorePlayer(const FString& PlayerKey, const TArray<FMonsterDexEntry>& Entries)
{
	if (!Save || PlayerKey.IsEmpty()) return;
	FMonsterDexPlayerRecord* Record = Save->Players.FindByPredicate([&PlayerKey](const FMonsterDexPlayerRecord& R) { return R.PlayerKey == PlayerKey; });
	if (!Record) Record = &Save->Players.AddDefaulted_GetRef();
	Record->PlayerKey = PlayerKey;
	Record->Entries = Entries;
	// ponytail: writes the whole slot on every change, the way the dungeon record does; batch on a timer if kills get frequent.
	if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0))
	{
		UE_LOG(SONHEIM, Error, TEXT("[MonsterDex] could not write slot %s"), SlotName);
	}
}
