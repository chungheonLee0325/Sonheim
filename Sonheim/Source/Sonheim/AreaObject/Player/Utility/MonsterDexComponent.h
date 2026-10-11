#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/MonsterDex/MonsterDexTypes.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/UI/Notice/NoticeSubsystem.h"
#include "MonsterDexComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMonsterDexChanged);

/**
 * One player's monster book: how many of each species the player defeated and captured, and which page rewards were paid.
 * Lives on the player's PlayerState. The server counts and pays; the owner gets the pages through replication, and the host's
 * save keeps them between sessions under the player's unique net id.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UMonsterDexComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMonsterDexComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: reads the player's saved pages, once the player's unique net id is known. */
	void LoadSaved();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MonsterDex")
	void ServerClaimReward(int32 AreaObjectID, int32 Tier);

	UFUNCTION(BlueprintPure, Category="MonsterDex")
	bool CanClaimReward(int32 AreaObjectID, int32 Tier) const;

	const FMonsterDexEntry* FindEntry(int32 AreaObjectID) const;
	UFUNCTION(BlueprintPure, Category="MonsterDex")
	const TArray<FMonsterDexEntry>& GetEntries() const { return Dex.Items; }

	/** Any page changed, on the server and on the owning client. */
	UPROPERTY(BlueprintAssignable, Category="MonsterDex")
	FOnMonsterDexChanged OnDexChanged;

private:
	void Record(int32 AreaObjectID, bool bCaptured);
	/** Marks the entry for replication, tells local listeners and writes the save. */
	void Commit(FMonsterDexEntry& Entry);
	/** On a headline the category is the big coloured line, the title the name under it. */
	void Notify(ENoticeSlot Slot, ENoticeTone Tone, const FText& Category, const FText& Title, const FText& Detail, const FText& Footer = FText()) const;
	/** Server: defeats and captures, as the PlayerState reports them. */
	void HandleDeed(EPlayerDeed Deed, int32 Id, int32 Count, FName Place);

	UPROPERTY(Replicated)
	FMonsterDexList Dex;
};
