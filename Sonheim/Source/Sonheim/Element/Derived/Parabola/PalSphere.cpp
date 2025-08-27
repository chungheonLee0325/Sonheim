#include "PalSphere.h"
#include "CollisionDebugDrawingPublic.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "Sonheim/UI/Widget/Player/CaptureProgressWidget.h"

APalSphere::APalSphere()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);

	// 월드 공간 UI
	CaptureWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CaptureWidget"));
	CaptureWidget->SetupAttachment(RootComponent);
	CaptureWidget->SetWidgetSpace(EWidgetSpace::Screen);
	CaptureWidget->SetDrawAtDesiredSize(true);
	CaptureWidget->SetVisibility(false);
	CaptureWidget->SetPivot(FVector2D(0.5f, 0.5f));
}

void APalSphere::BeginPlay()
{
	Super::BeginPlay();

	if (CaptureProgressWidgetClass)
	{
		CaptureWidget->SetWidgetClass(CaptureProgressWidgetClass);
	}

	TryBindToCaptureComp();
}

void APalSphere::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRevealDelegates();  
	Super::EndPlay(EndPlayReason);
}

void APalSphere::BindRevealDelegates(UCaptureProgressWidget* W)
{
	if (!W) return;
	
	W->OnSegmentFilled.RemoveDynamic(this, &APalSphere::OnWidgetSegmentFilled);
	W->OnRevealFinished.RemoveDynamic(this, &APalSphere::OnWidgetRevealFinished);

	W->OnSegmentFilled.AddDynamic(this, &APalSphere::OnWidgetSegmentFilled);
	W->OnRevealFinished.AddDynamic(this, &APalSphere::OnWidgetRevealFinished);

	BoundRevealWidget = W;
}

void APalSphere::UnbindRevealDelegates()
{
	if (BoundRevealWidget.IsValid())
	{
		BoundRevealWidget->OnSegmentFilled.RemoveDynamic(this, &APalSphere::OnWidgetSegmentFilled);
		BoundRevealWidget->OnRevealFinished.RemoveDynamic(this, &APalSphere::OnWidgetRevealFinished);
		BoundRevealWidget.Reset();
	}
}

void APalSphere::OnRep_Owner()
{
	Super::OnRep_Owner();
	TryBindToCaptureComp();
}

void APalSphere::TryBindToCaptureComp()
{
	if (CaptureComp.IsValid()) return;

	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) PawnOwner = GetInstigator();

	ASonheimPlayer* Player = Cast<ASonheimPlayer>(PawnOwner);
	if (!Player) return;

	if (UPalCaptureComponent* Comp = Player->FindComponentByClass<UPalCaptureComponent>())
	{
		CaptureComp = Comp;
		Comp->OnCaptureReveal.AddDynamic(this, &APalSphere::HandleCaptureReveal);
	}
}

void APalSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APalSphere::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
                             FAttackData* AttackData)
{
	FVector CameraLocation;
	FRotator CameraRotation;
	Cast<ASonheimPlayer>(Caster)->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector CameraForward = CameraRotation.Vector();

	FVector firePos = Caster->GetMesh()->GetSocketLocation("Weapon_R");
	FVector targetPos = firePos + CameraForward * 1200.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Caster);
	FHitResult OutHitResult;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		firePos,
		targetPos,
		ECC_Visibility,
		QueryParams
	);
	if (bHit && Caster->bShowDebug)
	{
		TArray<struct FHitResult> OutHitResults;
		DrawLineTraces(GetWorld(), firePos, targetPos, OutHitResults, 3.0f);
		DrawDebugSphere(GetWorld(), OutHitResult.Location, 20.f, 20, FColor::Red, false, 2.0f);
		DrawDebugSphere(GetWorld(), OutHitResult.GetActor()->GetActorLocation(), 20.f, 20, FColor::Blue, false, 2.0f);
	}

	m_Caster = Caster;
	m_Target = Target;
	//LOG_SCREEN("Target Location : %f %f %f",targetPos.X, targetPos.Y, targetPos.Z);
	m_TargetLocation = bHit ? OutHitResult.Location : targetPos;
	//LOG_SCREEN("OutHitResult Location : %f %f %f",OutHitResult.Location.X, OutHitResult.Location.Y, OutHitResult.Location.Z);
	//LOG_SCREEN("Hit Location : %f %f %f",m_TargetLocation.X, m_TargetLocation.Y, m_TargetLocation.Z);
	m_AttackData = AttackData;

	TryBindToCaptureComp();

	// Collision
	Root->SetCollisionProfileName(TEXT("MonsterProjectile"));

	float ArcValue{FMath::RandRange(0.8f, 0.9f)};
	Root->AddImpulse(Fire(m_Caster, m_Target, m_TargetLocation, ArcValue));
}

FVector APalSphere::Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue)
{
	// Todo : 가까우면 너무 느림, 속도 최소값 정하긴 해야할듯
	FVector StartLoc{Caster->GetMesh()->GetSocketLocation("Weapon_R")};
	FVector TargetLoc{StartLoc + GetActorForwardVector() * (GetActorLocation() - TargetLocation).Length()};
	FVector OutVelocity{FVector::ZeroVector};
	if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, OutVelocity, StartLoc, TargetLoc,
	                                                          GetWorld()->GetGravityZ(), ArcValue))
	{
		if (m_Caster->bShowDebug)
		{
			FPredictProjectilePathParams PredictParams(5.f, StartLoc, OutVelocity, 15.f);
			PredictParams.DrawDebugTime = 2.f;
			PredictParams.DrawDebugType = EDrawDebugTrace::Type::ForDuration;
			PredictParams.OverrideGravityZ = GetWorld()->GetGravityZ();
			FPredictProjectilePathResult Result;
			UGameplayStatics::PredictProjectilePath(this, PredictParams, Result);
		}
	}

	return OutVelocity;
}

void APalSphere::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                const FHitResult& SweepResult)
{
	ABaseMonster* pal = Cast<ABaseMonster>(OtherActor);
	if (m_Caster == OtherActor || !bCanHit || pal == nullptr || !pal->CanAttack(m_Caster))
	{
		return;
	}

	// 캡처 가능한지 확인
	if (!pal->CanCapture())
	{
		return;
	}

	FHitResult Hit;
	if (m_Caster && pal)
	{
		bCanHit = false;
		LastHitPal = pal;

		// === 서버: 포획 시도 & 히트 대상 복제 ===
		if (HasAuthority())
		{
			LastHitPal = pal;
			CheckPalCatch(Cast<ASonheimPlayer>(m_Caster), pal);
			FTimerHandle TimerHandle;
			// Impulse 이후 freeze
			GetWorldTimerManager().SetTimer(TimerHandle, this, &APalSphere::FreezeSphereWhileShowing, 0.2f, false);
		}


		HandleBeginOverlap(m_Caster, pal);
	}
}


void APalSphere::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
}

void APalSphere::CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target)
{
	if (!Caster || !Target)
		return;

	// PalCaptureComponent를 통해 포획 시도
	if (CaptureComp.Get())
	{
		CaptureComp->AttemptCapture(Target, this);
	}

	// 던진 구체에 물리 효과 추가
	int randX = FMath::RandRange(-80, 80);
	int randY = FMath::RandRange(-80, 80);
	Root->AddImpulse(FVector(randX, randY, 900));
}


void APalSphere::HandleCaptureReveal(class ABaseMonster* Pal, const FPalCaptureRevealParams& Params, APalSphere* SourceSphere)
{
	if (SourceSphere != this) return;
	if (!IsValid(Pal)) return;

	// 서버가 내려준 파라미터로 그대로 재생
	StartCaptureProgressReveal(
		Params.Guess,
		Params.bSuccess,
		Params.Segments,
		Params.SegmentTime,
		Params.InterStageDelay,
		Params.StartDelay,
		Params.FailStageOverride);

	// 생명주기도 서버 계산과 동일하게 세팅
	const int32 K = Params.bSuccess ? Params.Segments : FMath::Max(0, Params.FailStageOverride);
	const float Life = Params.StartDelay + K * Params.SegmentTime
		+ (K > 0 ? (K - 1) * Params.InterStageDelay : 0.f)
		+ Params.EndDelay + 0.1f;
	SetLifeSpan(Life);
}

void APalSphere::StartCaptureProgressReveal(float Guess01, bool bSuccess, int32 Segments,
                                     float SegmentTime, float InterDelay,
                                     float StartDelay, int32 FailStageOverride)
{
	if (!CaptureWidget) return;

	CaptureWidget->SetVisibility(true);
	BP_OnCaptureRevealStart(); // 시작 VFX/애니

	if (auto* UW = Cast<UCaptureProgressWidget>(CaptureWidget->GetWidget()))
	{
		// 세그먼트 끝마다 까닥/이펙트, 종료 시 End VFX
		UnbindRevealDelegates();
		BindRevealDelegates(UW);

		UW->PlayCaptureProgressReveal(Guess01, bSuccess, Segments, SegmentTime,
		                            InterDelay, StartDelay, FailStageOverride);
	}
}

void APalSphere::FreezeSphereWhileShowing()
{
	// 히트 후 UI가 흔들리지 않도록 충돌/물리 잠깐 OFF
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(RootComponent))
	{
		Prim->SetSimulatePhysics(false);
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APalSphere::OnWidgetSegmentFilled(int32 SegmentIndex)
{
	// 세그먼트 완료시: BP 이벤트 + 까닥 회전
	BP_OnSegmentFilled(SegmentIndex);
	NodOnce();
}

void APalSphere::OnWidgetRevealFinished(bool bSuccess)
{
	// 연출 종료: BP 이벤트 (닫힘 애니/VFX)
	BP_OnCaptureRevealEnd(bSuccess);

	UnbindRevealDelegates();
}

void APalSphere::NodOnce()
{
	// 짧게 Roll(+Angle) → 잠시 후 원위치
	SkeletalMesh->AddLocalRotation(FRotator(0.f, 0.f, NodAngleDeg));
	if (NodTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(NodTimerHandle);
	}
	GetWorldTimerManager().SetTimer(NodTimerHandle, this, &APalSphere::NodReturn, NodReturnDelay, false);
}

void APalSphere::NodReturn()
{
	SkeletalMesh->AddLocalRotation(FRotator(0.f, 0.f, -NodAngleDeg));
}
