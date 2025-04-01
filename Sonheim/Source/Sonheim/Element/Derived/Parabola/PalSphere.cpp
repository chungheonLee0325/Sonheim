// Fill out your copyright notice in the Description page of Project Settings.


#include "PalSphere.h"

#include "CollisionDebugDrawingPublic.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

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
	//LOG_SCREEN("Target Location : %f %f %f",targetPos.X, targetPos.Y, targetPos.Z);
	m_TargetLocation = bHit ? OutHitResult.Location : targetPos;
	//LOG_SCREEN("OutHitResult Location : %f %f %f",OutHitResult.Location.X, OutHitResult.Location.Y, OutHitResult.Location.Z);
	//LOG_SCREEN("Hit Location : %f %f %f",m_TargetLocation.X, m_TargetLocation.Y, m_TargetLocation.Z);
	m_AttackData = AttackData;

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

	FHitResult Hit;
	if (m_Caster && pal)
	{
		bCanHit = false;

		CheckPalCatch(Cast<ASonheimPlayer>(m_Caster), pal);

		HandleBeginOverlap(m_Caster, pal);
		//DestroySelf();
	}
}

void APalSphere::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
}

void APalSphere::CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target)
{
	if (Target->PartnerOwner != nullptr)
	{
		FLog::Log("This Pal is a owned pal");
	}
	int randNum = FMath::RandRange(1, 10);

	// 70 % 확률 포획
	if (randNum <= 5)
	{
		Target->SetPartnerOwner(Caster);
	}
	else
	{
		Target->DeactivateMonster();
	}
}
