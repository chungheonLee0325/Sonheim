#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "Sonheim/MonsterDex/MonsterDexData.h"
#include "Sonheim/Utilities/ItemChangeReason.h"

#include "MonsterDexComponent.generated.h"

class ASonheimPlayerState;
class UInventoryComponent;

USTRUCT(BlueprintType)
struct FMonsterDexEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 MonsterID = 0;

	UPROPERTY()
	int32 KillCount = 0;

	UPROPERTY()
	int32 CaptureCount = 0;

	UPROPERTY()
	TArray<int32> ClaimedRewardTierIndices;

	bool IsRewardClaimed(int32 TierIndex) const
	{
		return ClaimedRewardTierIndices.Contains(TierIndex);
	}
};

USTRUCT()
struct FRepMonsterDexList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMonsterDexEntry> Items;

	UPROPERTY(NotReplicated)
	class UMonsterDexComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMonsterDexEntry, FRepMonsterDexList>(Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FRepMonsterDexList> : public TStructOpsTypeTraitsBase2<FRepMonsterDexList>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMonsterDexChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UMonsterDexComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterDexComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_DexList();

	// ===== Server-side notifications =====
	void NotifyKilled(int32 MonsterID);
	void NotifyCaptured(int32 MonsterID);

	// ===== Reward claim =====
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MonsterDex")
	void ServerClaimReward(int32 MonsterID, int32 TierIndex);

	UFUNCTION(BlueprintPure, Category="MonsterDex")
	bool CanClaimReward(int32 MonsterID, int32 TierIndex) const;

	UFUNCTION(BlueprintPure, Category="MonsterDex")
	const TArray<FMonsterDexEntry>& GetDexEntries() const { return RepDex.Items; }

	UPROPERTY(BlueprintAssignable, Category="MonsterDex")
	FOnMonsterDexChanged OnDexChanged;

private:
	ASonheimPlayerState* ResolvePlayerState() const;
	UInventoryComponent* ResolveInventory() const;

	FMonsterDexEntry* FindEntry(int32 MonsterID);
	const FMonsterDexEntry* FindEntry(int32 MonsterID) const;
	FMonsterDexEntry* FindOrAddEntry(int32 MonsterID);

	bool MeetsTierRequirement(const FMonsterDexEntry& Entry, const FMonsterDexRewardTier& Tier) const;
	const FRewardDef* ResolveRewardDef(const FMonsterDexRewardTier& Tier) const;
	void GrantRewardDef(const FRewardDef& Reward);
	bool TryAutoClaimRewards(FMonsterDexEntry& Entry, const FMonsterDexData& Def);

private:
	UPROPERTY(ReplicatedUsing=OnRep_DexList)
	FRepMonsterDexList RepDex;
};
