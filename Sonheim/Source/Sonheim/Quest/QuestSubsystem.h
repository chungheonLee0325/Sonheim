#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuestTypes.h"
#include "QuestSubsystem.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct FQuestPlayerRecord
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString PlayerKey;
	UPROPERTY(BlueprintReadOnly) TArray<FQuestRecord> Quests;
};

UCLASS()
class SONHEIM_API UQuestSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly) TArray<FQuestPlayerRecord> Players;
};

/**
 * The quests, from DT_Quest, and every player's quest records, kept between sessions on the machine that hosts, the way
 * UMonsterDexSubsystem keeps the monster book. Only the server reads and writes the save.
 */
UCLASS(Config=Game)
class SONHEIM_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static const TCHAR* SlotName;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FQuestData* FindQuest(int32 QuestID) const;
	/** Every quest, in QuestID order. */
	const TArray<FQuestData>& GetQuests() const { return Quests; }

	TArray<FQuestRecord> LoadPlayer(const FString& PlayerKey) const;
	void StorePlayer(const FString& PlayerKey, const TArray<FQuestRecord>& Records);

private:
	UPROPERTY(Config) TSoftObjectPtr<UDataTable> QuestTable;
	TArray<FQuestData> Quests;
	UPROPERTY(Transient) TObjectPtr<UQuestSaveGame> Save;
};
