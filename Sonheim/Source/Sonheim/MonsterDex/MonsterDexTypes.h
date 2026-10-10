#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Sonheim/Rewards/RewardTypes.h"
#include "MonsterDexTypes.generated.h"

/** A reward a species' page pays once, after enough of it were defeated and captured. A count of 0 asks nothing of that kind. */
USTRUCT(BlueprintType)
struct FMonsterDexRewardTier
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RequiredKills = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RequiredCaptures = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Exp = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FItemReward> Items;
};

/** One species' page, a row of DT_MonsterDex. The picture and capturability come from the species' own dt_AreaObject row. */
USTRUCT(BlueprintType)
struct FMonsterDexData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AreaObjectID = 0;
	/** The page's number in the book, shown as No. 001; pages are listed in this order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Number = 0;
	/** The species' name as the book shows it; a DataTable row's own name is its key, so this is not called Name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMonsterDexRewardTier> RewardTiers;
	/** The species' character class, whose mesh the book's stage shows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<class ACharacter> DisplayClass;
};

/** What one player has done to one species. Replicated to its owner and kept in the save as it is. */
USTRUCT(BlueprintType)
struct FMonsterDexEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 AreaObjectID = 0;
	UPROPERTY(BlueprintReadOnly) int32 KillCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 CaptureCount = 0;
	/** Bit i is set once tier i was paid. */
	UPROPERTY(BlueprintReadOnly) int32 ClaimedTiers = 0;

	bool IsClaimed(const int32 Tier) const { return Tier >= 0 && Tier < 32 && (ClaimedTiers & (1 << Tier)) != 0; }
	bool Meets(const FMonsterDexRewardTier& Tier) const { return KillCount >= Tier.RequiredKills && CaptureCount >= Tier.RequiredCaptures; }

	void PostReplicatedAdd(const struct FMonsterDexList& List);
	void PostReplicatedChange(const struct FMonsterDexList& List);
};

USTRUCT()
struct FMonsterDexList : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY() TArray<FMonsterDexEntry> Items;
	UPROPERTY(NotReplicated, Transient) TObjectPtr<class UMonsterDexComponent> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FMonsterDexEntry, FMonsterDexList>(Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FMonsterDexList> : public TStructOpsTypeTraitsBase2<FMonsterDexList>
{
	enum { WithNetDeltaSerializer = true };
};
