#include "BossMonster.h"

#include "BossFSM.h"
#include "BossTelegraph.h"
#include "EngineUtils.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
}

ABossMonster::ABossMonster()
{
	m_AiFSM = CreateDefaultSubobject<UBossFSM>(TEXT("FSM"));
	TelegraphClass = ABossTelegraph::StaticClass();
	// A boss holds its ground: hits do not push it around.
	m_KnockBackForceMultiplier = 0.f;
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
	OnBossStatusChanged.Broadcast(Status);
}

void ABossMonster::OnRep_Status()
{
	OnBossStatusChanged.Broadcast(Status);
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
		if (UAnimMontage* Montage = Anim->GetCurrentActiveMontage()) Anim->Montage_JumpToSection(Section, Montage);
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

void ABossMonster::LandStrike(const FBossStrike& Strike, const FTransform& Where, const TArray<FVector>& Spots, AAreaObject* Target, const int32 ProjectileCount)
{
	if (!HasAuthority()) return;
	// Projectiles keep a pointer to their attack; the pattern asset holds it for as long as the boss lives.
	FAttackData& Attack = const_cast<FAttackData&>(Strike.Attack);
	if (Strike.Projectile && ProjectileCount > 0)
	{
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
	for (TActorIterator<AAreaObject> It(GetWorld()); It; ++It)
	{
		AAreaObject* Other = *It;
		if (Other == this || Other->IsDie() || !CanAttack(Other)) continue;
		if (!IsInside(Strike, Where, ABossTelegraph::FeetTransform(Other, 0.f).GetLocation())) continue;
		FHitResult Hit;
		Hit.Location = Hit.ImpactPoint = Other->GetActorLocation();
		CalcDamage(Attack, this, Other, Hit);
	}
	if (Attack.FireVFX_N) MulticastStrikeEffect(Attack.FireVFX_N, Where.GetLocation(), Attack.VFXScale);
}
