#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/Quest/QuestTypes.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "QuestComponent.generated.h"

struct FInventoryItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestsChanged);

/**
 * One player's quests: which are taken, the step each is at and the count of each open objective. Lives on the player's
 * PlayerState. The server counts from the player's deeds, inventory and use of quest targets, and pays; the owner gets the
 * records through replication, and the host's save keeps them between sessions under the player's save key.
 *
 * Accepting and reporting go through the server with the quest target the player stands at, which must be the quest's giver or
 * receiver and within reach. A finished quest is marked before it pays, so a repeated report pays nothing.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UQuestComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: reads the player's saved quests, once the player's save key is known. */
	void LoadSaved();
	/** Server: the player used a quest target. Open Talk objectives on it are done and Deliver objectives take what the player
	 * holds. Returns whether any count changed. */
	bool Interact(AActor* Target);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest") void ServerAccept(int32 QuestID, AActor* Giver);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest") void ServerTurnIn(int32 QuestID, AActor* Receiver);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest") void ServerAbandon(int32 QuestID);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Quest") void ServerTrack(int32 QuestID, bool bTracked);

	/** Not taken yet and every prerequisite finished. */
	UFUNCTION(BlueprintPure, Category="Quest") bool CanAccept(int32 QuestID) const;
	/** The quests this target offers the player now. */
	UFUNCTION(BlueprintPure, Category="Quest") TArray<int32> GetOffers(FName Giver) const;
	/** The player's finished quests that wait to be reported to this target. */
	UFUNCTION(BlueprintPure, Category="Quest") TArray<int32> GetReports(FName Receiver) const;
	UFUNCTION(BlueprintPure, Category="Quest") const TArray<FQuestRecord>& GetRecords() const { return Quests.Items; }
	const FQuestRecord* FindRecord(int32 QuestID) const;

	/** Server, for testing and design: puts a quest in a state as if played there; Active starts it over. */
	void DebugSet(int32 QuestID, EQuestState State);

	/** Any record changed, on the server and on the owning client. */
	UPROPERTY(BlueprintAssignable, Category="Quest") FOnQuestsChanged OnQuestsChanged;

	/** How close a player must stand to a giver or receiver to accept or report. */
	UPROPERTY(EditAnywhere, Category="Quest") float ReachDistance = 600.f;

private:
	void HandleDeed(EPlayerDeed Deed, int32 Id, int32 Count, FName Place);
	UFUNCTION() void HandleInventory(const TArray<FInventoryItem>& Items);
	/** Adds to the open objectives Gain says how much, for every active quest; marks what changed and settles it. */
	void Advance(TFunctionRef<int32(const FQuestObjective&)> Gain);
	/** Recounts held items, opens the next step while the current one is done, and ends the quest after its last step. */
	void Settle(FQuestRecord& Record, const FQuestData& Quest);
	void Finish(FQuestRecord& Record, const FQuestData& Quest);
	bool IsAt(AActor* Target, FName Name) const;
	void Changed(FQuestRecord& Record);
	void Notify(const FText& Title, const FText& Detail) const;
	const FQuestData* QuestOf(int32 QuestID) const;
	FQuestRecord* Find(int32 QuestID);
	class ASonheimPlayerState* Owner() const;

	UPROPERTY(Replicated) FQuestList Quests;
};
