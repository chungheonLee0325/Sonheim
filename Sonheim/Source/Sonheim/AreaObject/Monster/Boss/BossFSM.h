#pragma once

#include "CoreMinimal.h"
#include "BossMonster.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "BossFSM.generated.h"

class ABossTelegraph;

/** The boss's brain, run on the server. It sleeps until woken, picks patterns by distance, weight and cooldown, plays their montages,
 * marks and lands their strikes on time, rests after a run of patterns, falls when enough damage lands, and roars into phase 2.
 * The boss moves through its own stages (EBossStage) rather than through state objects. */
UCLASS()
class SONHEIM_API UBossFSM : public UBaseAiFSM
{
	GENERATED_BODY()
public:
	UBossFSM();
	virtual void InitStatePool() override;
	virtual void UpdateState(float DeltaSeconds) override;
	/** Wakes a sleeping boss, and brings a fighting one closer to a knockdown. */
	void OnDamaged(float Damage);
	void Defeat();
	/** A capture hides the boss while it plays out; a failed one brings the boss back where it left off. */
	void Pause();
	void Resume();
	/** Makes a pattern the boss's next one, whatever the distance and cooldown. */
	bool Perform(const FGameplayTag& PatternId);

private:
	struct FStrikeRun
	{
		bool bMarked = false;
		bool bLanded = false;
		/** Where a strike on the target lands; a strike on the boss lands wherever the boss then stands. */
		FTransform Where;
		/** The spots projectiles are thrown at. */
		TArray<FVector> Spots;
		TArray<TWeakObjectPtr<ABossTelegraph>> Marks;
	};

	ABossMonster* Boss() const;
	double Now() const;
	float Tempo() const;
	int32 ProjectileCount(const FBossStrike& Strike) const;
	void Enter(EBossStage Stage, const FGameplayTag& ActionId, float Seconds);
	void Wake();
	void KnockDown();
	void Fight(float DeltaSeconds);
	int32 ChoosePattern(const AAreaObject* NewTarget) const;
	void Begin(int32 Index, AAreaObject* NewTarget);
	void RunPattern(float DeltaSeconds);
	void Mark(const FBossStrike& Strike, FStrikeRun& Run, const AAreaObject* Victim);
	void Leap(const FBossPattern& Pattern);
	void EndPattern(bool bCut);
	AAreaObject* PickTarget() const;
	void Face(const FVector& Location, float DeltaSeconds);
	void MoveTo(const AActor* Goal, const FVector& Location);
	void StopMoving();

	int32 PatternIndex = INDEX_NONE;
	float PatternClock = 0.f;
	TArray<bool> CuesDone;
	TArray<FStrikeRun> Strikes;
	bool bLeapt = false;
	TWeakObjectPtr<AAreaObject> Target;
	TMap<FGameplayTag, double> ReadyAt;
	FGameplayTag LastPattern;
	/** The pattern Perform asked for, which the boss starts next. */
	int32 QueuedPattern = INDEX_NONE;
	int32 PatternsSinceRest = 0;
	double StageEndsAt = 0;
	bool bGettingUp = false;
	bool bPaused = false;
	double PausedAt = 0;
	/** What the boss walks to: a target, or its own spot while MoveGoal is empty and bMovingHome is set. */
	TWeakObjectPtr<const AActor> MoveGoal;
	bool bMovingHome = false;
};
