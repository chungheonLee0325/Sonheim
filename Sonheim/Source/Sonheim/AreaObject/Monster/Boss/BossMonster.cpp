#include "BossMonster.h"

#include "BossFSM.h"
#include "BossTelegraph.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/Element/BaseElement.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BossSleeping, "Boss.State.Sleeping", "The boss sleeps until a player comes near or hits it");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BossWaking, "Boss.State.Waking", "The boss wakes up and roars");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BossRoaring, "Boss.State.Roaring", "The boss roars as its second phase begins");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BossResting, "Boss.State.Resting", "The boss catches its breath and can be captured");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_BossDown, "Boss.State.Down", "The boss lies knocked down and can be captured");

namespace
{
	// Someone on a ledge above or below a strike's area is out of its reach.
	constexpr float StrikeReachZ = 300.f;
	// A section jump is a cut in the montage; the anim graph's Inertialization node blends it over this long.
	constexpr float SectionBlendSeconds = 0.15f;
	// The rage aura brightens up to this many times as the health falls from phase 2 to nothing, and dims while the boss is spent.
	constexpr float RageBrightening = 2.f;
	constexpr float SpentRage = 0.35f;

	FLinearColor Brighter(const FLinearColor& Color, const float Scale)
	{
		return FLinearColor(Color.R * Scale, Color.G * Scale, Color.B * Scale, Color.A);
	}
}

ABossMonster::ABossMonster()
{
	m_AiFSM = CreateDefaultSubobject<UBossFSM>(TEXT("FSM"));
	TelegraphClass = ABossTelegraph::StaticClass();
	// A boss holds its ground: hits do not push it around.
	m_KnockBackForceMultiplier = 0.f;
	// The brain turns the boss to face its target, walking or not.
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ABossMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABossMonster, Status);
}

void ABossMonster::BeginPlay()
{
	Super::BeginPlay();
	// Every machine puts the boss to sleep itself: a multicast sent as it spawns can reach a client before the boss does.
	if (Patterns && Status.Stage == EBossStage::Sleeping) PlayLocal(Patterns->WakeMontage, UBossPatternDataAsset::SleepSection, 1.f);
	if (HasAuthority())
	{
		FBossStatus Sleeping = Status;
		Sleeping.ActionId = TAG_BossSleeping;
		SetStatus(Sleeping);
	}
}

UBossFSM* ABossMonster::Brain() const
{
	return Cast<UBossFSM>(m_AiFSM);
}

void ABossMonster::SetStatus(const FBossStatus& NewStatus)
{
	if (Status == NewStatus) return;
	Status = NewStatus;
	ForceNetUpdate();
	RefreshLook();
	OnBossStatusChanged.Broadcast(Status);
}

void ABossMonster::OnRep_Status()
{
	RefreshLook();
	OnBossStatusChanged.Broadcast(Status);
}

double ABossMonster::ServerNow() const
{
	const UWorld* World = GetWorld();
	return World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void ABossMonster::RefreshLook()
{
	if (!Patterns || GetNetMode() == NM_DedicatedServer) return;
	// The pattern under way gathers its charge from its start until its release; a new run of a pattern, or none, ends the last one.
	const FBossPattern* Pattern = Patterns->FindPattern(Status.ActionId);
	const bool bCharges = Pattern && Pattern->ChargeEffect && Pattern->ChargeEndSeconds > 0.f && Status.ActionStartServerTime > 0.0;
	if (!bCharges || ChargeFrom != Status.ActionStartServerTime) StopCharge();
	const float Tempo = Status.Phase >= 2 ? Patterns->PhaseTwoTempo : 1.f;
	if (bCharges && ChargeFrom != Status.ActionStartServerTime && ServerNow() < Status.ActionStartServerTime + Pattern->ChargeEndSeconds / Tempo)
	{
		ChargeFrom = Status.ActionStartServerTime;
		ChargeUntil = ChargeFrom + Pattern->ChargeEndSeconds / Tempo;
		for (const FName Socket : Pattern->ChargeSockets)
			if (UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(Pattern->ChargeEffect, GetMesh(), Socket, FVector::ZeroVector,
				FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true))
				ChargeEffects.Add(Effect);
	}
	// From phase 2 until it falls for good, the rage crackles over the whole body.
	const bool bRage = Patterns->RageEffect && Status.Phase >= 2 && Status.Stage != EBossStage::Defeated && !IsDie();
	if (bRage && !RageAura)
	{
		RageAura = UNiagaraFunctionLibrary::SpawnSystemAttached(Patterns->RageEffect, GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true);
		if (RageAura) UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(RageAura, TEXT("User.SkeletalMesh"), GetMesh());
	}
	else if (!bRage && RageAura)
	{
		RageAura->Deactivate();
		RageAura = nullptr;
	}
	if (bRage && !RageGlow && Patterns->RageOverlay)
	{
		RageGlow = UMaterialInstanceDynamic::Create(Patterns->RageOverlay, this);
		GetMesh()->SetOverlayMaterial(RageGlow);
	}
	else if (!bRage && RageGlow)
	{
		GetMesh()->SetOverlayMaterial(nullptr);
		RageGlow = nullptr;
	}
}

void ABossMonster::StopCharge()
{
	// Deactivated, a charge lets its particles fade and then goes by itself.
	for (UNiagaraComponent* Effect : ChargeEffects)
		if (IsValid(Effect)) Effect->Deactivate();
	ChargeEffects.Reset();
	ChargeFrom = 0;
}

void ABossMonster::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Patterns) return;
	if (!ChargeEffects.IsEmpty())
	{
		// Deeper and brighter toward the release, faster at the end; in phase 2 it runs toward the rage color.
		const float Alpha = FMath::Clamp(float((ServerNow() - ChargeFrom) / FMath::Max(ChargeUntil - ChargeFrom, 0.01)), 0.f, 1.f);
		const FLinearColor Release = Status.Phase >= 2 ? Patterns->RageColor : Patterns->ChargeReleaseColor;
		const FLinearColor Color = FMath::Lerp(Patterns->ChargeColor, Release, Alpha * Alpha);
		const float Size = FMath::Lerp(Patterns->ChargeSize.X, Patterns->ChargeSize.Y, Alpha);
		for (UNiagaraComponent* Effect : ChargeEffects)
		{
			if (!IsValid(Effect)) continue;
			Effect->SetVariableLinearColor(TEXT("User.Color"), Color);
			Effect->SetVariableFloat(TEXT("User.Size"), Size);
		}
		if (Alpha >= 1.f) StopCharge();
	}
	if (IsValid(RageAura) || RageGlow)
	{
		const float Health = GetMaxHP() > 0.f ? GetHP() / GetMaxHP() : 1.f;
		const float Anger = FMath::Clamp((Patterns->PhaseTwoHealth - Health) / FMath::Max(Patterns->PhaseTwoHealth, 0.01f), 0.f, 1.f);
		const bool bSpent = Status.Stage == EBossStage::Resting || Status.Stage == EBossStage::Down;
		const float Rage = (1.f + RageBrightening * Anger) * (bSpent ? SpentRage : 1.f);
		if (IsValid(RageAura)) RageAura->SetVariableLinearColor(TEXT("User.Color"), Brighter(Patterns->RageColor, Rage));
		if (RageGlow)
		{
			RageGlow->SetVectorParameterValue(TEXT("Color"), Patterns->RageColor);
			RageGlow->SetScalarParameterValue(TEXT("Strength"), Rage);
		}
	}
}

bool ABossMonster::PerformPattern(const FGameplayTag PatternId)
{
	UBossFSM* Fsm = Brain();
	return Fsm && Fsm->Perform(PatternId);
}

bool ABossMonster::CanCapture() const
{
	return Super::CanCapture() && Status.IsVulnerable();
}

void ABossMonster::ActivateMonster()
{
	Super::ActivateMonster();
	if (UBossFSM* Fsm = Brain()) Fsm->Resume();
}

void ABossMonster::DeactivateMonster()
{
	Super::DeactivateMonster();
	if (UBossFSM* Fsm = Brain()) Fsm->Pause();
}

float ABossMonster::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Damage > 0.f && HasAuthority() && !IsDie())
		if (UBossFSM* Fsm = Brain()) Fsm->OnDamaged(Damage);
	return Damage;
}

void ABossMonster::OnDie_Implementation()
{
	Super::OnDie_Implementation();
	// The boss falls where it stands, instead of rolling away the way a small monster does.
	GetCapsuleComponent()->SetSimulatePhysics(false);
	if (Patterns) PlayLocal(Patterns->DownMontage, UBossPatternDataAsset::FallSection, 1.f);
	if (HasAuthority())
		if (UBossFSM* Fsm = Brain()) Fsm->Defeat();
}

void ABossMonster::PlayLocal(UAnimMontage* Montage, const FName Section, const float PlayRate)
{
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !Montage) return;
	Anim->Montage_Play(Montage, PlayRate);
	if (!Section.IsNone()) Anim->Montage_JumpToSection(Section, Montage);
}

void ABossMonster::PlayMontage(UAnimMontage* Montage, const FName Section, const float PlayRate)
{
	MulticastPlayMontage(Montage, Section, PlayRate);
}

void ABossMonster::MulticastPlayMontage_Implementation(UAnimMontage* Montage, const FName Section, const float PlayRate)
{
	PlayLocal(Montage, Section, PlayRate);
}

void ABossMonster::JumpToSection(const FName Section)
{
	MulticastJumpToSection(Section);
}

void ABossMonster::MulticastJumpToSection_Implementation(const FName Section)
{
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		if (UAnimMontage* Montage = Anim->GetCurrentActiveMontage())
		{
			Anim->RequestSlotGroupInertialization(Montage->GetGroupName(), SectionBlendSeconds);
			Anim->Montage_JumpToSection(Section, Montage);
		}
}

void ABossMonster::StopMontage()
{
	MulticastStopMontage();
}

void ABossMonster::MulticastStopMontage_Implementation()
{
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr) Anim->Montage_Stop(0.2f);
}

void ABossMonster::MulticastStrikeEffect_Implementation(UNiagaraSystem* Effect, const FVector Location, const float Scale)
{
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, Location, FRotator::ZeroRotator, FVector(Scale));
}

ABossTelegraph* ABossMonster::PlaceMark(const FBossStrike& Strike, const FTransform& Where, const bool bFollow, const float Seconds)
{
	if (!HasAuthority() || Strike.Shape == EBossAreaShape::None || !TelegraphClass) return nullptr;
	ABossTelegraph* Mark = GetWorld()->SpawnActorDeferred<ABossTelegraph>(TelegraphClass, Where, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Mark) return nullptr;
	Mark->Mark.Shape = Strike.Shape;
	Mark->Mark.Radius = Strike.Radius;
	Mark->Mark.InnerRadius = Strike.InnerRadius;
	Mark->Mark.HalfAngle = Strike.HalfAngle;
	Mark->Mark.HalfWidth = Strike.HalfWidth;
	Mark->Mark.Seconds = Seconds;
	if (bFollow)
	{
		Mark->Mark.Follow = this;
		Mark->Mark.FollowForward = Strike.ForwardOffset;
	}
	Mark->FinishSpawning(Where);
	Mark->SetLifeSpan(Seconds + 0.25f);
	return Mark;
}

bool ABossMonster::IsInside(const FBossStrike& Strike, const FTransform& Where, const FVector& Point)
{
	const FVector Local = Where.InverseTransformPositionNoScale(Point);
	if (FMath::Abs(Local.Z) > StrikeReachZ) return false;
	const float Distance = FVector2D(Local.X, Local.Y).Size();
	switch (Strike.Shape)
	{
	case EBossAreaShape::Circle: return Distance <= Strike.Radius;
	case EBossAreaShape::Ring: return Distance >= Strike.InnerRadius && Distance <= Strike.Radius;
	case EBossAreaShape::Cone: return Distance <= Strike.Radius && FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Y), Local.X)) <= Strike.HalfAngle;
	case EBossAreaShape::Line: return Local.X >= 0.f && Local.X <= Strike.Radius && FMath::Abs(Local.Y) <= Strike.HalfWidth;
	default: return false;
	}
}

void ABossMonster::LandStrike(const FBossStrike& Strike, const FTransform& Where, const TArray<FVector>& Spots, AAreaObject* Target, const int32 Count)
{
	if (!HasAuthority()) return;
	// Projectiles keep a pointer to their attack; the pattern asset holds it for as long as the boss lives.
	FAttackData& Attack = const_cast<FAttackData&>(Strike.Attack);
	if (Strike.Projectile)
	{
		const int32 ProjectileCount = FMath::Max(1, Count);
		const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius();
		const float Step = ProjectileCount > 1 ? Strike.SpreadDegrees / (ProjectileCount - 1) : 0.f;
		const float FirstYaw = Where.Rotator().Yaw - (ProjectileCount > 1 ? Strike.SpreadDegrees * 0.5f : 0.f);
		for (int32 Index = 0; Index < ProjectileCount; ++Index)
		{
			const bool bAtSpot = Strike.Aim == EBossProjectileAim::Location;
			if (bAtSpot && !Spots.IsValidIndex(Index)) break;
			// A thrown projectile is given its spot, a flying one its direction: the element classes read either.
			const FRotator Facing = bAtSpot ? (Spots[Index] - Muzzle).GetSafeNormal2D().Rotation() : FRotator(0.f, FirstYaw + Step * Index, 0.f);
			ABaseElement* Element = GetWorld()->SpawnActor<ABaseElement>(Strike.Projectile, Muzzle, Facing);
			if (!Element) continue;
			Element->SetOwner(this);
			Element->InitElement(this, Target, bAtSpot ? Spots[Index] : Facing.Vector(), &Attack);
		}
		return;
	}
	// The area at Where, or the same area at each spot; someone where two of them overlap is hit once.
	TArray<FTransform> Areas;
	if (Spots.IsEmpty()) Areas.Add(Where);
	for (const FVector& Spot : Spots) Areas.Add(FTransform(Where.GetRotation(), Spot));
	for (TActorIterator<AAreaObject> It(GetWorld()); It; ++It)
	{
		AAreaObject* Other = *It;
		if (Other == this || Other->IsDie() || !CanAttack(Other)) continue;
		const FVector Feet = ABossTelegraph::FeetTransform(Other, 0.f).GetLocation();
		if (!Areas.ContainsByPredicate([&Strike, &Feet](const FTransform& Area) { return IsInside(Strike, Area, Feet); })) continue;
		FHitResult Hit;
		Hit.Location = Hit.ImpactPoint = Other->GetActorLocation();
		CalcDamage(Attack, this, Other, Hit);
	}
	if (Attack.FireVFX_N)
		for (const FTransform& Area : Areas) MulticastStrikeEffect(Attack.FireVFX_N, Area.GetLocation(), Attack.VFXScale);
}
