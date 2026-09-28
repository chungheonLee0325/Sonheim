#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "BossPatternDataAsset.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "BossMonster.generated.h"

class ABossTelegraph;
class UBossFSM;
class UNiagaraSystem;

// What the boss does between its patterns; the dungeon screen names each of them.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BossSleeping);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BossWaking);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BossRoaring);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BossResting);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_BossDown);

UENUM(BlueprintType)
enum class EBossStage : uint8
{
	Sleeping,
	Waking,
	Fighting,
	Roaring,
	Resting,
	Down,
	Defeated,
};

USTRUCT(BlueprintType)
struct FBossStatus
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Boss") EBossStage Stage = EBossStage::Sleeping;
	UPROPERTY(BlueprintReadOnly, Category="Boss") int32 Phase = 1;
	/** A pattern's id while the boss performs it, Boss.State.* while it sleeps, wakes, roars, rests or lies down, and empty in between. */
	UPROPERTY(BlueprintReadOnly, Category="Boss") FGameplayTag ActionId;
	/** Server times the action started at and ends at. */
	UPROPERTY(BlueprintReadOnly, Category="Boss") double ActionStartServerTime = 0;
	UPROPERTY(BlueprintReadOnly, Category="Boss") double ActionEndServerTime = 0;
	/** Damage toward the next knockdown, 0 to 1. */
	UPROPERTY(BlueprintReadOnly, Category="Boss") float Break = 0.f;
	/** Worn out at low health, the boss rests: the only time it can be captured. Lying knocked down only takes free hits. */
	bool IsVulnerable() const { return Stage == EBossStage::Resting; }
	bool operator==(const FBossStatus& Other) const = default;
};

/** A boss: it fights in patterns whose strikes mark the ground before they land, wakes when approached, turns fiercer below a share of
 * its health, and can be captured only while it rests or lies knocked down. UBossFSM runs it on the server. */
UCLASS()
class SONHEIM_API ABossMonster : public ABaseMonster
{
	GENERATED_BODY()
public:
	ABossMonster();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss") TObjectPtr<UBossPatternDataAsset> Patterns;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss") TSubclassOf<ABossTelegraph> TelegraphClass;

	UFUNCTION(BlueprintPure, Category="Boss") FBossStatus GetBossStatus() const { return Status; }
	/** On every machine: the server as it sets the status, a client as the status arrives. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossStatusChanged, const FBossStatus&);
	FOnBossStatusChanged OnBossStatusChanged;

	/** Makes a pattern the boss's next one, whatever the distance and cooldown, so it can be watched on its own in play, the way the
	 * boss checks do. False for a pattern the boss does not have. */
	UFUNCTION(BlueprintCallable, Category="Boss") bool PerformPattern(FGameplayTag PatternId);
	/** Whether a pal sphere would take the boss now: only while it rests worn out at low health. */
	UFUNCTION(BlueprintPure, Category="Boss") bool IsCapturable() const { return CanCapture(); }

	virtual bool CanCapture() const override;
	virtual void ActivateMonster() override;
	virtual void DeactivateMonster() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// The server's side: what the brain has the boss do.
	void SetStatus(const FBossStatus& NewStatus);
	void PlayMontage(UAnimMontage* Montage, FName Section, float PlayRate);
	void JumpToSection(FName Section);
	void StopMontage();
	/** Marks a strike's area at Where: its spot on the ground, or the boss's feet when bFollow keeps the mark on the boss. */
	ABossTelegraph* PlaceMark(const FBossStrike& Strike, const FTransform& Where, bool bFollow, float Seconds);
	/** Lands a strike: damages everyone in its area at Where, or in the same area at each of Spots, or fires Count projectiles along
	 * the spread or at Spots. */
	void LandStrike(const FBossStrike& Strike, const FTransform& Where, const TArray<FVector>& Spots, AAreaObject* Target, int32 Count);
	/** Whether a point on the ground lies in a strike's area placed at Where. */
	static bool IsInside(const FBossStrike& Strike, const FTransform& Where, const FVector& Point);

protected:
	virtual void BeginPlay() override;
	virtual void OnDie_Implementation() override;
	UFUNCTION() void OnRep_Status();
	UFUNCTION(NetMulticast, Reliable) void MulticastPlayMontage(UAnimMontage* Montage, FName Section, float PlayRate);
	UFUNCTION(NetMulticast, Reliable) void MulticastJumpToSection(FName Section);
	UFUNCTION(NetMulticast, Reliable) void MulticastStopMontage();
	UFUNCTION(NetMulticast, Unreliable) void MulticastStrikeEffect(UNiagaraSystem* Effect, FVector Location, float Scale);
	void PlayLocal(UAnimMontage* Montage, FName Section, float PlayRate);
	UBossFSM* Brain() const;
	UPROPERTY(ReplicatedUsing=OnRep_Status) FBossStatus Status;
};
