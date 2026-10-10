#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MonsterDexTypes.h"
#include "MonsterDexSubsystem.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct FMonsterDexPlayerRecord
{
	GENERATED_BODY()
	/** The player's unique net id as a string, the same on every visit from the same account. */
	UPROPERTY(BlueprintReadOnly) FString PlayerKey;
	UPROPERTY(BlueprintReadOnly) TArray<FMonsterDexEntry> Entries;
};

UCLASS()
class SONHEIM_API UMonsterDexSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly) TArray<FMonsterDexPlayerRecord> Players;
};

/**
 * The book's pages, from DT_MonsterDex, and every player's progress in it, kept between sessions on the machine that hosts.
 * Only the server reads and writes the save; clients get their own progress through UMonsterDexComponent's replication.
 */
UCLASS(Config=Game)
class SONHEIM_API UMonsterDexSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static const TCHAR* SlotName;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FMonsterDexData* FindPage(int32 AreaObjectID) const;
	/** Every page, in book order. */
	const TArray<FMonsterDexData>& GetPages() const { return Pages; }

	TArray<FMonsterDexEntry> LoadPlayer(const FString& PlayerKey) const;
	/** Replaces the player's record and writes the slot. */
	void StorePlayer(const FString& PlayerKey, const TArray<FMonsterDexEntry>& Entries);

private:
	UPROPERTY(Config) TSoftObjectPtr<UDataTable> PageTable;
	TArray<FMonsterDexData> Pages;
	UPROPERTY(Transient) TObjectPtr<UMonsterDexSaveGame> Save;
};
