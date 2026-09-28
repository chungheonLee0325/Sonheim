#include "BossFSM.h"

#include "AIController.h"
#include "BossTelegraph.h"
#include "EngineUtils.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

namespace
{
	// How fast the boss turns to its target while a pattern tracks it.
	constexpr float TurnDegreesPerSecond = 300.f;
	// How close the boss walks up to its target when no pattern reaches it yet.
	constexpr float ChaseAcceptance = 250.f;
}

UBossFSM::UBossFSM()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBossFSM::InitStatePool()
{
	// The boss moves through its own stages, so there are no state objects to build.
}

ABossMonster* UBossFSM::Boss() const
{
	return Cast<ABossMonster>(m_Owner);
}

double UBossFSM::Now() const
{
	const UWorld* World = GetWorld();
	return World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

float UBossFSM::Tempo() const
{
	const ABossMonster* Owner = Boss();
	return Owner->GetBossStatus().Phase >= 2 ? Owner->Patterns->PhaseTwoTempo : 1.f;
}

int32 UBossFSM::ProjectileCount(const FBossStrike& Strike) const
{
	if (Strike.ProjectileCount <= 0) return 0;
	const ABossMonster* Owner = Boss();
	return Strike.ProjectileCount + (Owner->GetBossStatus().Phase >= 2 ? Owner->Patterns->PhaseTwoExtraProjectiles : 0);
}

void UBossFSM::Enter(const EBossStage Stage, const FGameplayTag& ActionId, const float Seconds)
{
	ABossMonster* Owner = Boss();
	FBossStatus Status = Owner->GetBossStatus();
	Status.Stage = Stage;
	Status.ActionId = ActionId;
	Status.ActionStartServerTime = Seconds > 0.f ? Now() : 0.0;
	Status.ActionEndServerTime = Seconds > 0.f ? Now() + Seconds : 0.0;
	// Falling down spends the damage that brought it about.
	if (Stage == EBossStage::Down) Status.Break = 0.f;
	StageEndsAt = Now() + Seconds;
	bGettingUp = false;
	Owner->SetStatus(Status);
}

void UBossFSM::UpdateState(const float DeltaSeconds)
{
	ABossMonster* Owner = Boss();
	// A captured boss belongs to a player now, and the boss's brain fights only for itself.
	if (!Owner || !Owner->HasAuthority() || !Owner->Patterns || bPaused || Owner->IsDie() || Owner->PartnerOwner) return;
	const UBossPatternDataAsset& Data = *Owner->Patterns;
	switch (Owner->GetBossStatus().Stage)
	{
	case EBossStage::Sleeping:
		for (TActorIterator<ASonheimPlayer> It(GetWorld()); It; ++It)
			if (!It->IsDie() && FVector::Dist2D(It->GetActorLocation(), Owner->GetActorLocation()) <= Data.WakeRadius)
			{
				Wake();
				break;
			}
		return;
	case EBossStage::Waking:
	case EBossStage::Roaring:
	case EBossStage::Resting:
		if (Now() >= StageEndsAt) Enter(EBossStage::Fighting, FGameplayTag(), 0.f);
		return;
	case EBossStage::Down:
		if (!bGettingUp && Now() >= StageEndsAt - Data.GetUpSeconds)
		{
			bGettingUp = true;
			Owner->JumpToSection(UBossPatternDataAsset::GetUpSection);
		}
		if (Now() >= StageEndsAt) Enter(EBossStage::Fighting, FGameplayTag(), 0.f);
		return;
	case EBossStage::Fighting:
		Fight(DeltaSeconds);
		return;
	default:
		return;
	}
}

void UBossFSM::Wake()
{
	ABossMonster* Owner = Boss();
	if (Owner->GetBossStatus().Stage != EBossStage::Sleeping) return;
	Owner->PlayMontage(Owner->Patterns->WakeMontage, UBossPatternDataAsset::WakeSection, 1.f);
	Enter(EBossStage::Waking, TAG_BossWaking, Owner->Patterns->WakeSeconds);
}

void UBossFSM::OnDamaged(const float Damage)
{
	ABossMonster* Owner = Boss();
	if (!Owner || !Owner->Patterns || bPaused) return;
	const FBossStatus Status = Owner->GetBossStatus();
	if (Status.Stage == EBossStage::Sleeping)
	{
		Wake();
		return;
	}
	// Only a fighting boss moves toward a knockdown: damage while it wakes, roars, rests or lies there counts for nothing.
	const float MaxHP = Owner->GetMaxHP();
	if (Status.Stage != EBossStage::Fighting || MaxHP <= 0.f) return;
	FBossStatus Next = Status;
	Next.Break = FMath::Min(1.f, Status.Break + Damage / (MaxHP * Owner->Patterns->DownAfterDamage));
	Owner->SetStatus(Next);
	if (Next.Break >= 1.f) KnockDown();
}

void UBossFSM::KnockDown()
{
	ABossMonster* Owner = Boss();
	EndPattern(true);
	StopMoving();
	Owner->PlayMontage(Owner->Patterns->DownMontage, UBossPatternDataAsset::FallSection, 1.f);
	Enter(EBossStage::Down, TAG_BossDown, Owner->Patterns->DownSeconds);
}

void UBossFSM::Defeat()
{
	if (!Boss()) return;
	EndPattern(true);
	StopMoving();
	Enter(EBossStage::Defeated, FGameplayTag(), 0.f);
}

void UBossFSM::Pause()
{
	if (bPaused || !Boss()) return;
	EndPattern(true);
	StopMoving();
	bPaused = true;
	PausedAt = Now();
}

void UBossFSM::Resume()
{
	if (!bPaused) return;
	bPaused = false;
	// The time the capture took does not come out of a rest or a knockdown.
	const double Paused = Now() - PausedAt;
	StageEndsAt += Paused;
	ABossMonster* Owner = Boss();
	FBossStatus Status = Owner->GetBossStatus();
	if (Status.ActionEndServerTime > 0)
	{
		Status.ActionStartServerTime += Paused;
		Status.ActionEndServerTime += Paused;
	}
	// A pattern cut off by the capture is over; the boss picks the next one.
	if (Status.Stage == EBossStage::Fighting) Status.ActionId = FGameplayTag();
	Owner->SetStatus(Status);
}

bool UBossFSM::Perform(const FGameplayTag& PatternId)
{
	ABossMonster* Owner = Boss();
	if (!Owner || !Owner->HasAuthority() || !Owner->Patterns) return false;
	const int32 Index = Owner->Patterns->Patterns.IndexOfByPredicate([&PatternId](const FBossPattern& Pattern) { return Pattern.PatternId == PatternId; });
	if (Index == INDEX_NONE) return false;
	QueuedPattern = Index;
	return true;
}

AAreaObject* UBossFSM::PickTarget() const
{
	ABossMonster* Owner = Boss();
	const float ArenaRadius = Owner->Patterns->ArenaRadius;
	AAreaObject* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ASonheimPlayer> It(GetWorld()); It; ++It)
	{
		ASonheimPlayer* Player = *It;
		if (Player->IsDie() || !Owner->CanAttack(Player)) continue;
		if (FVector::Dist2D(Player->GetActorLocation(), Owner->GetSpawnLocation()) > ArenaRadius) continue;
		const float Distance = FVector::Dist2D(Player->GetActorLocation(), Owner->GetActorLocation());
		if (Distance < NearestDistance)
		{
			Nearest = Player;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void UBossFSM::Fight(const float DeltaSeconds)
{
	if (PatternIndex != INDEX_NONE)
	{
		RunPattern(DeltaSeconds);
		return;
	}
	ABossMonster* Owner = Boss();
	const UBossPatternDataAsset& Data = *Owner->Patterns;
	FBossStatus Status = Owner->GetBossStatus();
	// Phase 2 begins between patterns, with a roar.
	if (Status.Phase < 2 && Owner->GetHP() <= Owner->GetMaxHP() * Data.PhaseTwoHealth)
	{
		StopMoving();
		Status.Phase = 2;
		Owner->SetStatus(Status);
		Owner->PlayMontage(Data.WakeMontage, UBossPatternDataAsset::RoarSection, 1.f);
		Enter(EBossStage::Roaring, TAG_BossRoaring, Data.RoarSeconds);
		return;
	}
	if (PatternsSinceRest >= Data.PatternsBeforeRest + Status.Phase - 1)
	{
		StopMoving();
		PatternsSinceRest = 0;
		// The rest montage plays over the whole rest, whatever its own length.
		const float Length = Data.RestMontage ? Data.RestMontage->GetPlayLength() : Data.RestSeconds;
		Owner->PlayMontage(Data.RestMontage, NAME_None, Length / FMath::Max(Data.RestSeconds, 0.1f));
		Enter(EBossStage::Resting, TAG_BossResting, Data.RestSeconds);
		return;
	}
	AAreaObject* NewTarget = PickTarget();
	if (!NewTarget)
	{
		// Nobody within reach: back to its spot, to wait there.
		if (FVector::Dist2D(Owner->GetActorLocation(), Owner->GetSpawnLocation()) > ChaseAcceptance) MoveTo(nullptr, Owner->GetSpawnLocation());
		else StopMoving();
		return;
	}
	const int32 Index = QueuedPattern != INDEX_NONE ? QueuedPattern : ChoosePattern(NewTarget);
	QueuedPattern = INDEX_NONE;
	if (Index != INDEX_NONE) Begin(Index, NewTarget);
	// No pattern reaches the target from here: close in.
	else MoveTo(NewTarget, NewTarget->GetActorLocation());
}

int32 UBossFSM::ChoosePattern(const AAreaObject* NewTarget) const
{
	const ABossMonster* Owner = Boss();
	const UBossPatternDataAsset& Data = *Owner->Patterns;
	const int32 Phase = Owner->GetBossStatus().Phase;
	const float Distance = FVector::Dist2D(Owner->GetActorLocation(), NewTarget->GetActorLocation());
	const double Time = Now();
	TArray<int32> Candidates;
	float Total = 0.f;
	// The same pattern twice in a row only when nothing else fits.
	for (const bool bAllowRepeat : {false, true})
	{
		for (int32 Index = 0; Index < Data.Patterns.Num(); ++Index)
		{
			const FBossPattern& Pattern = Data.Patterns[Index];
			if (Pattern.MinPhase > Phase || Pattern.Weight <= 0.f || !Pattern.Montage) continue;
			if (Distance < Pattern.MinRange || Distance > Pattern.MaxRange) continue;
			if (const double* Ready = ReadyAt.Find(Pattern.PatternId); Ready && *Ready > Time) continue;
			if (!bAllowRepeat && Pattern.PatternId == LastPattern) continue;
			Candidates.Add(Index);
			Total += Pattern.Weight;
		}
		if (!Candidates.IsEmpty()) break;
	}
	if (Candidates.IsEmpty()) return INDEX_NONE;
	float Roll = FMath::FRandRange(0.f, Total);
	for (const int32 Index : Candidates)
	{
		Roll -= Data.Patterns[Index].Weight;
		if (Roll <= 0.f) return Index;
	}
	return Candidates.Last();
}

void UBossFSM::Begin(const int32 Index, AAreaObject* NewTarget)
{
	ABossMonster* Owner = Boss();
	const FBossPattern& Pattern = Owner->Patterns->Patterns[Index];
	StopMoving();
	Target = NewTarget;
	PatternIndex = Index;
	PatternClock = 0.f;
	bLeapt = false;
	CuesDone.Init(false, Pattern.Cues.Num());
	Strikes.Reset();
	Strikes.SetNum(Pattern.Strikes.Num());
	Owner->SetAnimRootMotionTranslationScale(Pattern.RootMotionScale);
	Owner->PlayMontage(Pattern.Montage, NAME_None, Tempo());
	Enter(EBossStage::Fighting, Pattern.PatternId, Pattern.Seconds / Tempo());
}

void UBossFSM::RunPattern(const float DeltaSeconds)
{
	ABossMonster* Owner = Boss();
	const FBossPattern& Pattern = Owner->Patterns->Patterns[PatternIndex];
	const int32 Phase = Owner->GetBossStatus().Phase;
	PatternClock += DeltaSeconds * Tempo();
	AAreaObject* Victim = Target.Get();
	if (Victim && Victim->IsDie()) Victim = nullptr;
	if (Victim && !bLeapt && PatternClock < Pattern.TrackSeconds) Face(Victim->GetActorLocation(), DeltaSeconds);

	for (int32 Index = 0; Index < Pattern.Cues.Num(); ++Index)
	{
		if (CuesDone[Index] || PatternClock < Pattern.Cues[Index].Seconds) continue;
		CuesDone[Index] = true;
		Owner->JumpToSection(Pattern.Cues[Index].Section);
	}
	for (int32 Index = 0; Index < Pattern.Strikes.Num(); ++Index)
	{
		const FBossStrike& Strike = Pattern.Strikes[Index];
		FStrikeRun& Run = Strikes[Index];
		if (Strike.MinPhase > Phase || Run.bLanded) continue;
		if (!Run.bMarked && PatternClock >= Strike.MarkSeconds) Mark(Strike, Run, Victim);
		if (Run.bMarked && PatternClock >= Strike.StrikeSeconds)
		{
			Run.bLanded = true;
			const FTransform Where = Strike.Anchor == EBossAreaAnchor::Boss ? ABossTelegraph::FeetTransform(Owner, Strike.ForwardOffset) : Run.Where;
			Owner->LandStrike(Strike, Where, Run.Spots, Victim, ProjectileCount(Strike));
		}
	}
	// The leap goes after the marks of its moment, so it can come down on the one it placed.
	if (Pattern.LeapEndSeconds > 0.f && !bLeapt && PatternClock >= Pattern.LeapStartSeconds) Leap(Pattern);
	if (PatternClock >= Pattern.Seconds) EndPattern(false);
}

void UBossFSM::Mark(const FBossStrike& Strike, FStrikeRun& Run, const AAreaObject* Victim)
{
	ABossMonster* Owner = Boss();
	Run.bMarked = true;
	const float Seconds = (Strike.StrikeSeconds - Strike.MarkSeconds) / Tempo();
	if (Strike.Anchor == EBossAreaAnchor::Boss)
	{
		Run.Marks.Add(Owner->PlaceMark(Strike, ABossTelegraph::FeetTransform(Owner, Strike.ForwardOffset), true, Seconds));
		return;
	}
	// On the target: the spot it stands on now, facing away from the boss. The mark stays there and so does the strike.
	const FTransform BossFeet = ABossTelegraph::FeetTransform(Owner, 0.f);
	const FVector From = BossFeet.GetLocation();
	const FVector Spot = Victim ? ABossTelegraph::FeetTransform(Victim, 0.f).GetLocation() : From + BossFeet.Rotator().Vector() * 600.f;
	const FVector Away = (Spot - From).GetSafeNormal2D();
	const FRotator Facing = Away.IsNearlyZero() ? BossFeet.Rotator() : Away.Rotation();
	Run.Where = FTransform(Facing, Spot + Facing.Vector() * Strike.ForwardOffset);
	const int32 Count = ProjectileCount(Strike);
	if (Count > 0 && Strike.Aim == EBossProjectileAim::Location)
	{
		// The spots a projectile is thrown at: the target's spot, turned across the spread around the boss.
		const float Distance = FVector::Dist2D(From, Run.Where.GetLocation());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Yaw = Facing.Yaw + (Count > 1 ? Strike.SpreadDegrees * (float(Index) / (Count - 1) - 0.5f) : 0.f);
			FVector Point = From + FRotator(0.f, Yaw, 0.f).Vector() * Distance;
			Point.Z = Run.Where.GetLocation().Z;
			Run.Spots.Add(Point);
			Run.Marks.Add(Owner->PlaceMark(Strike, FTransform(FRotator(0.f, Yaw, 0.f), Point), false, Seconds));
		}
		return;
	}
	Run.Marks.Add(Owner->PlaceMark(Strike, Run.Where, false, Seconds));
}

void UBossFSM::Leap(const FBossPattern& Pattern)
{
	bLeapt = true;
	ABossMonster* Owner = Boss();
	// Onto the pattern's first mark on the target, which shows where the boss comes down; without one, onto the target.
	const FVector Feet = ABossTelegraph::FeetTransform(Owner, 0.f).GetLocation();
	FVector Landing = Feet;
	const int32 Marked = Pattern.Strikes.IndexOfByPredicate([](const FBossStrike& Strike) { return Strike.Anchor == EBossAreaAnchor::Target; });
	if (Strikes.IsValidIndex(Marked) && Strikes[Marked].bMarked) Landing = Strikes[Marked].Where.GetLocation();
	else if (Target.IsValid()) Landing = ABossTelegraph::FeetTransform(Target.Get(), 0.f).GetLocation();
	else return;
	// A throw that comes down on the spot as the leap ends.
	const float Seconds = FMath::Max(0.1f, (Pattern.LeapEndSeconds - Pattern.LeapStartSeconds) / Tempo());
	const FVector Delta = Landing - Feet;
	FVector Velocity = Delta / Seconds;
	Velocity.Z = Delta.Z / Seconds - 0.5f * Owner->GetCharacterMovement()->GetGravityZ() * Seconds;
	StopMoving();
	Owner->SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));
	Owner->LaunchCharacter(Velocity, true, true);
}

void UBossFSM::EndPattern(const bool bCut)
{
	if (PatternIndex == INDEX_NONE) return;
	ABossMonster* Owner = Boss();
	const FBossPattern& Pattern = Owner->Patterns->Patterns[PatternIndex];
	// A cut pattern lands nothing more, so the marks still waiting go with it.
	if (bCut)
		for (FStrikeRun& Run : Strikes)
			for (const TWeakObjectPtr<ABossTelegraph>& Placed : Run.Marks)
				if (Placed.IsValid() && !Run.bLanded) Placed->Destroy();
	if (!bCut) ++PatternsSinceRest;
	ReadyAt.Add(Pattern.PatternId, Now() + Pattern.Cooldown);
	LastPattern = Pattern.PatternId;
	PatternIndex = INDEX_NONE;
	Strikes.Reset();
	Owner->SetAnimRootMotionTranslationScale(1.f);
	if (!bCut) Enter(EBossStage::Fighting, FGameplayTag(), 0.f);
}

void UBossFSM::Face(const FVector& Location, const float DeltaSeconds)
{
	ABossMonster* Owner = Boss();
	const FVector To = Location - Owner->GetActorLocation();
	if (To.IsNearlyZero()) return;
	const float Yaw = FMath::FixedTurn(Owner->GetActorRotation().Yaw, To.Rotation().Yaw, TurnDegreesPerSecond * DeltaSeconds);
	Owner->SetActorRotation(FRotator(0.f, Yaw, 0.f));
}

void UBossFSM::MoveTo(const AActor* Goal, const FVector& Location)
{
	AAIController* Controller = Cast<AAIController>(Boss()->GetController());
	if (!Controller) return;
	// A move under way keeps going unless the boss now walks somewhere else.
	const bool bSameGoal = Goal ? MoveGoal.Get() == Goal : bMovingHome;
	if (bSameGoal && Controller->GetMoveStatus() == EPathFollowingStatus::Moving) return;
	MoveGoal = Goal;
	bMovingHome = !Goal;
	if (Goal) Controller->MoveToActor(const_cast<AActor*>(Goal), ChaseAcceptance);
	else Controller->MoveToLocation(Location, ChaseAcceptance * 0.5f);
}

void UBossFSM::StopMoving()
{
	MoveGoal.Reset();
	bMovingHome = false;
	AAIController* Controller = Boss() ? Cast<AAIController>(Boss()->GetController()) : nullptr;
	if (Controller && Controller->GetMoveStatus() != EPathFollowingStatus::Idle) Controller->StopMovement();
}
