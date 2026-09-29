#include "BossFSM.h"

#include "AIController.h"
#include "BossTelegraph.h"
#include "EngineUtils.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

namespace
{
	// How fast the boss turns to its target while it moves about between patterns; a pattern sets its own rate.
	constexpr float IdleTurnDegreesPerSecond = 160.f;
	// How close the boss walks up to its target when no pattern reaches it yet.
	constexpr float ChaseAcceptance = 250.f;
	// A hop rises this high; it and a leap go up for RiseShare of their time and come down in the rest, fast.
	constexpr float HopHeight = 130.f;
	constexpr float HopSeconds = 0.55f;
	constexpr float RiseShare = 0.65f;
	// Between patterns, a target within SideHopRange draws a hop to its side this often.
	constexpr float SideHopRange = 1500.f;
	constexpr float SideHopChance = 0.4f;
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

int32 UBossFSM::StrikeCount(const FBossStrike& Strike) const
{
	// Phase 2 adds to every strike of several: more projectiles, more spots.
	const ABossMonster* Owner = Boss();
	const bool bSeveral = Strike.Projectile || Strike.Count > 1;
	return FMath::Max(1, Strike.Count) + (bSeveral && Owner->GetBossStatus().Phase >= 2 ? Owner->Patterns->PhaseTwoExtraCount : 0);
}

bool UBossFSM::TryExhaust()
{
	ABossMonster* Owner = Boss();
	const UBossPatternDataAsset& Data = *Owner->Patterns;
	const float Health = Owner->GetMaxHP() > 0.f ? Owner->GetHP() / Owner->GetMaxHP() : 1.f;
	// Every mark the health has fallen through counts as taken, so one big hit past two of them brings one rest, not two.
	bool bDue = false;
	while (ExhaustsTaken < Data.ExhaustHealth.Num() && Health <= Data.ExhaustHealth[ExhaustsTaken])
	{
		++ExhaustsTaken;
		bDue = true;
	}
	if (!bDue) return false;
	EndPattern(true);
	StopMoving();
	// The exhaust montage plays over the whole rest, whatever its own length.
	const float Length = Data.ExhaustMontage ? Data.ExhaustMontage->GetPlayLength() : Data.ExhaustSeconds;
	Owner->PlayMontage(Data.ExhaustMontage, NAME_None, Length / FMath::Max(Data.ExhaustSeconds, 0.1f));
	Enter(EBossStage::Resting, TAG_BossResting, Data.ExhaustSeconds);
	return true;
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
	if (!Owner || !Owner->HasAuthority()) return;
	UpdateFlight();
	// A captured boss belongs to a player now, and the boss's brain fights only for itself.
	if (!Owner->Patterns || bPaused || Owner->IsDie() || Owner->PartnerOwner) return;
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
	// Worn out at low health, it sinks down at once, whatever it was doing.
	if (TryExhaust()) return;
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
	if (TryExhaust()) return;
	AAreaObject* NewTarget = PickTarget();
	if (!NewTarget)
	{
		// Nobody within reach: back to its spot, facing the way it walks, to wait there.
		if (FVector::Dist2D(Owner->GetActorLocation(), Owner->GetSpawnLocation()) > ChaseAcceptance) MoveTo(nullptr, Owner->GetSpawnLocation());
		else StopMoving();
		const FVector Velocity = Owner->GetVelocity();
		if (Velocity.SizeSquared2D() > 1.f) Face(Owner->GetActorLocation() + Velocity, DeltaSeconds, IdleTurnDegreesPerSecond);
		return;
	}
	// Whatever it does, it keeps its eyes on its target; a hop lands before anything else starts.
	Face(NewTarget->GetActorLocation(), DeltaSeconds, IdleTurnDegreesPerSecond);
	if (bFlying) return;
	if (Now() < GapEndsAt)
	{
		if (GapMove == EGapMove::None)
		{
			// Too close: a hop back, or a stand where there is no room. Further off: now and then a hop to the side, else a walk in.
			const float Distance = FVector::Dist2D(Owner->GetActorLocation(), NewTarget->GetActorLocation());
			if (Distance < Data.BackOffRange) GapMove = Hop(NewTarget, true) ? EGapMove::Hop : EGapMove::Hold;
			else GapMove = Distance < SideHopRange && FMath::FRand() < SideHopChance && Hop(NewTarget, false) ? EGapMove::Hop : EGapMove::Walk;
		}
		if (GapMove == EGapMove::Walk) MoveTo(NewTarget, NewTarget->GetActorLocation());
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
	// It turns to its target while the pattern tracks it, and again before a strike that re-aims, from the landing of the one before.
	bool bTracking = PatternClock < Pattern.TrackSeconds;
	for (int32 Index = 1; !bTracking && Index < Pattern.Strikes.Num(); ++Index)
		bTracking = Pattern.Strikes[Index].bReaim && Strikes[Index - 1].bLanded && !Strikes[Index].bMarked;
	if (Victim && !bLeapt && bTracking) Face(Victim->GetActorLocation(), DeltaSeconds, Pattern.TurnDegreesPerSecond);

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
			Owner->LandStrike(Strike, Where, Run.Spots, Victim, StrikeCount(Strike));
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
	const int32 Count = StrikeCount(Strike);
	if (Strike.Projectile ? Strike.Aim == EBossProjectileAim::Location : Count > 1)
	{
		// Every spot gets its mark: the first on the target and the rest scattered around it, or all of them turned across the
		// spread around the boss at the target's distance.
		const FVector Center = Run.Where.GetLocation();
		const float Distance = FVector::Dist2D(From, Center);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FVector Point = Center;
			if (Strike.Scatter > 0.f)
			{
				if (Index > 0) Point += FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * FMath::FRandRange(0.35f, 1.f) * Strike.Scatter;
			}
			else
			{
				const float Yaw = Facing.Yaw + (Count > 1 ? Strike.SpreadDegrees * (float(Index) / (Count - 1) - 0.5f) : 0.f);
				Point = From + FRotator(0.f, Yaw, 0.f).Vector() * Distance;
				Point.Z = Center.Z;
			}
			Run.Spots.Add(Point);
			Run.Marks.Add(Owner->PlaceMark(Strike, FTransform(Facing, Point), false, Seconds));
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
	Owner->SetActorRotation(FRotator(0.f, (Landing - Feet).Rotation().Yaw, 0.f));
	Launch(Landing, FMath::Max(0.1f, (Pattern.LeapEndSeconds - Pattern.LeapStartSeconds) / Tempo()), Pattern.LeapHeight);
}

bool UBossFSM::Hop(const AAreaObject* Foe, const bool bBack)
{
	ABossMonster* Owner = Boss();
	const UBossPatternDataAsset& Data = *Owner->Patterns;
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Data.HopMontage || !Nav) return false;
	const FVector Feet = ABossTelegraph::FeetTransform(Owner, 0.f).GetLocation();
	FVector Away = (Feet - Foe->GetActorLocation()).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = -Owner->GetActorForwardVector();
	// Straight back or back and aside; one side or else the other. A spot off the floor, out of the arena or behind a wall is passed
	// over, and so is one the boss's own capsule would strike a pillar or a wall on the way to: the navigation mesh is laid for a
	// much thinner body. The players are no obstacle, the boss flies through them.
	const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossHop), false, Owner);
	FCollisionResponseParams Responses;
	Responses.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	const float Side = FMath::RandBool() ? 90.f : -90.f;
	for (const float Turn : bBack ? TArray<float>{0.f, 35.f, -35.f} : TArray<float>{Side, -Side})
	{
		FNavLocation Spot;
		FVector Blocked;
		FHitResult Hit;
		if (!Nav->ProjectPointToNavigation(Feet + Away.RotateAngleAxis(Turn, FVector::UpVector) * Data.HopDistance, Spot)) continue;
		if (FVector::Dist2D(Spot.Location, Owner->GetSpawnLocation()) > Data.ArenaRadius) continue;
		if (UNavigationSystemV1::NavigationRaycast(GetWorld(), Feet, Spot.Location, Blocked)) continue;
		const FVector Up(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() + 10.f);
		if (GetWorld()->SweepSingleByChannel(Hit, Feet + Up, Spot.Location + Up, FQuat::Identity, ECC_Pawn, Capsule->GetCollisionShape(), Query, Responses)) continue;
		Owner->PlayMontage(Data.HopMontage, NAME_None, Tempo());
		Launch(Spot.Location, HopSeconds / Tempo(), HopHeight);
		return true;
	}
	return false;
}

void UBossFSM::Launch(const FVector& Landing, const float Seconds, const float Height)
{
	ABossMonster* Owner = Boss();
	UCharacterMovementComponent* Move = Owner->GetCharacterMovement();
	// Two gravities make the arc: a lighter one carries the boss to its top in the first RiseShare of the time, a heavier one brings
	// it down onto Landing in the rest.
	const FVector Delta = Landing - ABossTelegraph::FeetTransform(Owner, 0.f).GetLocation();
	const float Top = FMath::Max(0.f, float(Delta.Z)) + Height;
	const float Rise = Seconds * RiseShare;
	const float Fall = Seconds - Rise;
	const float Gravity = FMath::Max(1.f, -GetWorld()->GetGravityZ());
	// It flies through the players on its way: coming down on someone's head, it would stand there.
	UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	if (!bFlying)
	{
		GroundGravityScale = Move->GravityScale;
		PawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
	}
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Move->GravityScale = 2.f * Top / (Rise * Rise) / Gravity;
	FallGravityScale = 2.f * (Top - Delta.Z) / (Fall * Fall) / Gravity;
	bFlying = true;
	bLeftGround = false;
	LaunchedAt = Now();
	FVector Velocity = Delta / Seconds;
	Velocity.Z = 2.f * Top / Rise;
	StopMoving();
	Owner->LaunchCharacter(Velocity, true, true);
}

void UBossFSM::UpdateFlight()
{
	if (!bFlying) return;
	ABossMonster* Owner = Boss();
	UCharacterMovementComponent* Move = Owner->GetCharacterMovement();
	if (Move->IsFalling())
	{
		bLeftGround = true;
		if (Move->Velocity.Z <= 0.f) Move->GravityScale = FallGravityScale;
		return;
	}
	// The launch takes hold on the movement's next update; one that never does is dropped.
	if (!bLeftGround && Now() - LaunchedAt < 0.5) return;
	bFlying = false;
	Move->GravityScale = GroundGravityScale;
	Owner->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, PawnResponse);
	// Touching down in a pattern or a hop, whenever that is: the landing plays now.
	if (bLeftGround && Owner->GetBossStatus().Stage == EBossStage::Fighting) Owner->JumpToSection(UBossPatternDataAsset::LandSection);
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
	if (!bCut)
		GapEndsAt = Now() + FMath::FRandRange(Owner->Patterns->GapSecondsMin, Owner->Patterns->GapSecondsMax) / Tempo();
	GapMove = EGapMove::None;
	ReadyAt.Add(Pattern.PatternId, Now() + Pattern.Cooldown);
	LastPattern = Pattern.PatternId;
	PatternIndex = INDEX_NONE;
	Strikes.Reset();
	Owner->SetAnimRootMotionTranslationScale(1.f);
	if (!bCut) Enter(EBossStage::Fighting, FGameplayTag(), 0.f);
}

void UBossFSM::Face(const FVector& Location, const float DeltaSeconds, const float DegreesPerSecond)
{
	ABossMonster* Owner = Boss();
	const FVector To = Location - Owner->GetActorLocation();
	if (To.IsNearlyZero()) return;
	const float Yaw = FMath::FixedTurn(Owner->GetActorRotation().Yaw, To.Rotation().Yaw, DegreesPerSecond * DeltaSeconds);
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
