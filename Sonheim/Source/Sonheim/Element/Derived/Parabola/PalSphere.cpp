#include "PalSphere.h"
#include "CollisionDebugDrawingPublic.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h"
#include "Sonheim/AreaObject/Player/Utility/PalManagementComponent.h"
#include "Sonheim/Utilities/LogMacro.h"

APalSphere::APalSphere()
{
	PrimaryActorTick.bCanEverTick = false;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshObject
	(TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/ResourceObject/Weapon/Palsphere/SK_Weapon_PalSphere_001_LOD0.SK_Weapon_PalSphere_001_LOD0'"));
	if (SkeletalMeshObject.Succeeded())
	{
		SkeletalMesh->SetSkeletalMesh(SkeletalMeshObject.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> ABP_PalSphere(TEXT("/Script/Engine.AnimBlueprintGeneratedClass'/Game/_BluePrint/Element/Parabola/ABP_PalSphere.ABP_PalSphere_C'"));
	if (ABP_PalSphere.Succeeded())
	{
		SkeletalMesh->SetAnimInstanceClass(ABP_PalSphere.Class);
	}
}

void APalSphere::BeginPlay()
{
	Super::BeginPlay();
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
	if (!HasAuthority())
	{
		return;
	}
	
	ABaseMonster* pal = Cast<ABaseMonster>(OtherActor);
	ASonheimPlayer* player = Cast<ASonheimPlayer>(m_Caster);
    
	if (!player || !pal || !bCanHit || bIsProcessingCapture || m_Caster == OtherActor)
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
	bIsProcessingCapture = true;
	TargetMonster = pal;

	// 포획 처리
	CheckPalCatch(player, pal);
}

void APalSphere::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
	// 지면이나 벽에 충돌 시 처리
	if (!bCanHit || Cast<ABaseMonster>(OtherActor))
		return;
}

void APalSphere::CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target)
{
    if (!Caster || !Target)
    {
        bIsProcessingCapture = false;
        return;
    }

    // 팰의 현재 상태 저장
    SavePalState(Target);

	Root->SetSimulatePhysics(true);
	int randX = FMath::RandRange(-80, 80);
	int randY = FMath::RandRange(-80, 80);
	Root->AddImpulse(FVector(randX, randY, 700));

	// 타겟을 임시로 숨김 (포획 애니메이션 중)
	Target->SetActorHiddenInGame(true);
	Target->SetActorEnableCollision(false);

	HandlePalSphereAnimation();

	// 떨어지는 애니메이션을 위한 타이머
	FTimerHandle FallTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(FallTimerHandle, [this, Caster, Target]()
	{
		// 바닥에 떨어진 위치로 이동
		FVector GroundLocation = Target->GetActorLocation();
		GroundLocation.Z = GetActorLocation().Z - 50.0f; // 바닥 위치 조정
		SetActorLocation(GroundLocation);
		
		// 물리 시뮬레이션 중지
		Root->SetSimulatePhysics(false);
		Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		// 흔들림 애니메이션 시작
		StartShakeAnimation(Caster, Target);
	}, 1.9f, false);
}

void APalSphere::StartShakeAnimation(ASonheimPlayer* Caster, ABaseMonster* Target)
{
	ShakeCount = 0;
	FVector OriginalLocation = GetActorLocation();
	
	// 흔들림 애니메이션
	GetWorld()->GetTimerManager().SetTimer(ShakeTimerHandle, [this, Caster, Target, OriginalLocation]()
	{
		ShakeCount++;
		
		// 3번 흔들림
		if (ShakeCount <= 3)
		{
			// 좌우로 흔들림
			float ShakeOffset = FMath::Sin(ShakeCount * PI) * 10.0f;
			FVector NewLocation = OriginalLocation;
			NewLocation.Y += ShakeOffset;
			SetActorLocation(NewLocation);
			
			// 흔들림 사운드 재생
			if (ShakeSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShakeSound, GetActorLocation());
			}
		}
		else
		{
			// 원위치로 복귀
			SetActorLocation(OriginalLocation);
			GetWorld()->GetTimerManager().ClearTimer(ShakeTimerHandle);
			
			// 포획 시도
			AttemptCapture(Caster, Target);
		}
	}, 0.5f, true);
}

void APalSphere::AttemptCapture(ASonheimPlayer* Caster, ABaseMonster* Target)
{
	if (!Caster || !Target)
		return;

	UPalCaptureComponent* CaptureComp = Caster->GetPalCaptureComponent();
	if (!CaptureComp)
		return;

	// 포획률 계산
	float CaptureRate = CaptureComp->CalculateCaptureRate(Target, SphereItemID);
	
	// 포획 시도
	float RandomValue = FMath::FRand();
	bool bSuccess = RandomValue <= CaptureRate;
	
	FLog::Log("Capture Attempt - Rate: %.2f%%, Roll: %.2f, Success: %s", 
		CaptureRate * 100.0f, RandomValue * 100.0f, bSuccess ? "YES" : "NO");
	
	// 결과 처리
	if (bSuccess)
	{
		ProcessCaptureSuccess(Caster, Target);
	}
	else
	{
		ProcessCaptureFailed(Target);
	}
}

void APalSphere::ProcessCaptureSuccess(ASonheimPlayer* Caster, ABaseMonster* Target)
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
	
	// PalManagementComponent를 통해 팰 등록
	if (UPalManagementComponent* PalMgmt = Caster->GetPalManagementComponent())
	{
		// 체력 회복
		Target->SetHPByRate(1.0f);
		
		// 팰 등록
		PalMgmt->RegisterPal(Target);
		
		// 성공 UI 알림
		if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(Caster->GetController()))
		{
			// UI 업데이트는 PalManagementComponent의 이벤트로 처리됨
		}
	}
	
	// 스피어 제거
	Destroy();
}

void APalSphere::ProcessCaptureFailed(ABaseMonster* Target)
{
	// 타임아웃 타이머 제거
	GetWorld()->GetTimerManager().ClearTimer(CaptureTimeoutHandle);
	GetWorld()->GetTimerManager().ClearTimer(ShakeTimerHandle);

	HandlePalSphereAnimation();
	
	// 실패 이펙트
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
	
	// 타겟 다시 표시
	Target->SetActorHiddenInGame(false);
	Target->SetActorEnableCollision(true);
	RestorePalState(Target);
	
	// 잠시 기절 상태로 만들기
	//Target->AddCondition(EConditionBitsType::Stunned, 2.0f);
	
	// 스피어 파괴 애니메이션
	// 스케일을 0으로 줄이면서 사라지기
	FTimerHandle DestroyTimerHandle;
	float DestroyTime = 1.0f;
	float ElapsedTime = 0.0f;
	
	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle, [this, DestroyTime, ElapsedTime]() mutable
	{
		ElapsedTime += 0.05f;
		float Alpha = ElapsedTime / DestroyTime;
		
		if (Alpha >= 1.0f)
		{
			Destroy();
		}
		else
		{
			SetActorScale3D(FVector(1.0f - Alpha));
		}
	}, 0.05f, true);
}

void APalSphere::SavePalState(ABaseMonster* Target)
{
	if (!Target) return;
    
	SavedPalState.OriginalLocation = Target->GetActorLocation();
	SavedPalState.OriginalRotation = Target->GetActorRotation();
}

void APalSphere::RestorePalState(ABaseMonster* Target)
{
	if (!Target) return;
    
	// 위치와 회전 복원
	Target->SetActorLocation(SavedPalState.OriginalLocation);
	Target->SetActorRotation(SavedPalState.OriginalRotation);
}
