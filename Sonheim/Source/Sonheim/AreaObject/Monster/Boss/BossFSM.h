#pragma once

#include "CoreMinimal.h"
#include "BossMonster.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "BossFSM.generated.h"

class ABossTelegraph;

/** The boss's brain, run on the server. It sleeps until woken, picks patterns by distance, weight and cooldown, plays their montages,
 * marks and lands their strikes on time, keeps moving between them while it faces its target, sinks down worn out at low health,
 * falls when enough damage lands, and roars into phase 2.
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
	int32 StrikeCount(const FBossStrike& Strike) const;
	/** Sinks the boss down worn out once its health falls to the next of its exhaust marks. */
	bool TryExhaust();
	void Enter(EBossStage Stage, const FGameplayTag& ActionId, float Seconds);
	void Wake();
	/** Plays the roar's sound on every machine Delay seconds from now; a roar still due is replaced. */
	void Roar(float Delay);
	void KnockDown();
	void Fight(float DeltaSeconds);
	int32 ChoosePattern(const AAreaObject* NewTarget) const;
	void Begin(int32 Index, AAreaObject* NewTarget);
	void RunPattern(float DeltaSeconds);
	void Mark(const FBossStrike& Strike, FStrikeRun& Run, const AAreaObject* Victim);
	void Leap(const FBossPattern& Pattern);
	/** Hops back from the foe, or to its side; false when there is no free spot to land on. */
	bool Hop(const AAreaObject* Foe, bool bBack);
	/** Throws the boss onto Landing, where it comes down Seconds later after rising Height over the higher end of the jump. */
	void Launch(const FVector& Landing, float Seconds, float Height);
	/** Past the top of a launch the boss falls with the heavier gravity; on the ground it gets its own back and plays Land. */
	void UpdateFlight();
	void EndPattern(bool bCut);
	AAreaObject* PickTarget() const;
	void Face(const FVector& Location, float DeltaSeconds, float DegreesPerSecond);
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
	/** The exhaust marks the boss's health has already fallen through. */
	int32 ExhaustsTaken = 0;
	/** Until then the boss moves about before it picks its next pattern; how, it picks as the gap begins. */
	double GapEndsAt = 0;
	enum class EGapMove : uint8 { None, Walk, Hold, Hop };
	EGapMove GapMove = EGapMove::None;
	/** A launch under way: when it began, whether it has left the ground, the gravity scale it falls with past the top, and the
	 * gravity scale and response to pawns it had before. */
	bool bFlying = false;
	bool bLeftGround = false;
	double LaunchedAt = 0;
	float FallGravityScale = 1.f;
	float GroundGravityScale = 1.f;
	ECollisionResponse PawnResponse = ECR_Block;
	double StageEndsAt = 0;
	/** When the roar's sound is due, 0 while none is. */
	double RoarAt = 0;
	bool bGettingUp = false;
	bool bPaused = false;
	double PausedAt = 0;
	/** What the boss walks to: a target, or its own spot while MoveGoal is empty and bMovingHome is set. */
	TWeakObjectPtr<const AActor> MoveGoal;
	bool bMovingHome = false;
};
