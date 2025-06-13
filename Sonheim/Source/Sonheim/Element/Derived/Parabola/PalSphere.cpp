#include "PalSphere.h"
#include "CollisionDebugDrawingPublic.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "Sonheim/Utilities/LogMacro.h"

APalSphere::APalSphere()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APalSphere::BeginPlay()
{
	Super::BeginPlay();
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
	m_TargetLocation = bHit ? OutHitResult.Location : targetPos;
	m_AttackData = AttackData;

	// Collision
	Root->SetCollisionProfileName(TEXT("MonsterProjectile"));

	float ArcValue{FMath::RandRange(0.8f, 0.9f)};
	Root->AddImpulse(Fire(m_Caster, m_Target, m_TargetLocation, ArcValue));
}

FVector APalSphere::Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue)
{
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
	ASonheimPlayer* player = Cast<ASonheimPlayer>(m_Caster);
	
	if (!player || !pal || !bCanHit || m_Caster == OtherActor)
	{
		return;
	}

	// 포획 가능 여부 확인
	if (UPalCaptureComponent* CaptureComp = player->GetPalCaptureComponent())
	{
		if (!CaptureComp->CanCapture(pal))
		{
			FLog::Log("Cannot capture this target");
			Destroy();
			return;
		}
	}

	bCanHit = false;
	TargetMonster = pal;

	// 충돌 시 물리 정지
	Root->SetSimulatePhysics(false);
	Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 포획 시도
	AttemptCapture(player, pal);
}

void APalSphere::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
	// 지면이나 벽에 충돌 시 처리
	if (!bCanHit || Cast<ABaseMonster>(OtherActor))
		return;
}

void APalSphere::AttemptCapture(ASonheimPlayer* Caster, ABaseMonster* Target)
{
	if (!Caster || !Target)
		return;

	UPalCaptureComponent* CaptureComp = Caster->GetPalCaptureComponent();
	if (!CaptureComp)
		return;

	// 포획 시도 전 몬스터 비활성화
	Target->SetActorHiddenInGame(true);
	Target->SetActorEnableCollision(false);
	
	// 스피어를 타겟 위치로 이동
	SetActorLocation(Target->GetActorLocation() + FVector(0, 0, 50));
	
	// 흔들림 애니메이션 시작
	ShakeCount = 0;
	GetWorld()->GetTimerManager().SetTimer(ShakeTimerHandle, [this, CaptureComp, Target]()
	{
		ShakeCount++;
		
		// 흔들림 효과
		float ShakeIntensity = FMath::Sin(ShakeCount * 0.5f) * 10.0f;
		AddActorWorldOffset(FVector(ShakeIntensity, 0, 0));
		
		// 3번 흔들린 후 포획 시도
		if (ShakeCount >= 6)
		{
			GetWorld()->GetTimerManager().ClearTimer(ShakeTimerHandle);
			
			// 포획 결과 이벤트 바인딩
			CaptureComp->OnCaptureAttempt.AddDynamic(this, &APalSphere::OnCaptureResult);
			
			// 포획 시도
			CaptureComp->AttemptCapture(Target, SphereItemID);
		}
	}, 0.3f, true);
}

void APalSphere::OnCaptureResult(ABaseMonster* Target, float CaptureRate, bool bSuccess)
{
	if (Target != TargetMonster)
		return;

	// 이벤트 언바인딩
	if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(m_Caster))
	{
		if (UPalCaptureComponent* CaptureComp = Player->GetPalCaptureComponent())
		{
			CaptureComp->OnCaptureAttempt.RemoveDynamic(this, &APalSphere::OnCaptureResult);
		}
	}

	// 포획 애니메이션 재생
	PlayCaptureAnimation(bSuccess);
	
	// 일정 시간 후 파괴
	SetLifeSpan(1.0f);
}

void APalSphere::PlayCaptureAnimation(bool bSuccess)
{
	if (bSuccess)
	{
		// 성공 이펙트
		if (CaptureSuccessEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CaptureSuccessEffect, 
				GetActorLocation(), FRotator::ZeroRotator, FVector(2.0f));
		}
		
		// 성공 사운드
		if (CaptureSuccessSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), CaptureSuccessSound, GetActorLocation());
		}
		
		// 스피어 사라지는 애니메이션
		// TODO: 스케일 애니메이션 또는 디졸브 효과
	}
	else
	{
		// 실패 이펙트 (스피어 깨짐)
		if (CaptureFailEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CaptureFailEffect, 
				GetActorLocation(), FRotator::ZeroRotator);
		}
		
		// 실패 사운드
		if (CaptureFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), CaptureFailSound, GetActorLocation());
		}
		
		// 스피어 파괴 애니메이션
		// TODO: 깨지는 애니메이션 또는 파티클 효과
	}
}