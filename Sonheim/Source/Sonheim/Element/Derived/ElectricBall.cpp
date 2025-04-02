// Fill out your copyright notice in the Description page of Project Settings.


#include "ElectricBall.h"

#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values
AElectricBall::AElectricBall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Root->SetSimulatePhysics(false);

}

// Called when the game starts or when spawned
void AElectricBall::BeginPlay()
{
	Super::BeginPlay();
	Root->OnComponentBeginOverlap.AddDynamic(this, &AElectricBall::OnBeginOverlap);
	Root->OnComponentHit.AddDynamic(this, &AElectricBall::OnComponentHit);

}

// Called every frame
void AElectricBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 플레이어 추적, 일정 시간 후 사라지게
	if (m_Target)
	{
		m_TargetLocation = m_Target->GetActorLocation() - GetActorLocation();
		m_TargetLocation.Normalize();
	}
	
	SetActorLocation(GetActorLocation() + DeltaTime * m_TargetLocation * Speed);

	FlowTime += DeltaTime;

	if (FlowTime > AttackTime)
	{
		FlowTime = 0.f;
		DestroySelf();
	}
	
	if (FVector::Dist(StartPos, GetActorLocation()) > Range)
	{
		DestroySelf();
	}
}

void AElectricBall::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
	FAttackData* AttackData)
{
	Super::InitElement(Caster, Target, TargetLocation, AttackData);
	StartPos = m_Caster->GetActorLocation();
	// m_TargetLocation : unit Vector
	EndPos = StartPos + m_TargetLocation * Range;
	
	if (m_Caster->bShowDebug)
	{
		DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Emerald, false, 1.f, 0, 1.f);
	}
}

void AElectricBall::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Super::OnBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	if (m_Caster == OtherActor)
	{
		return;
	}

	
	FHitResult Hit;
	if (m_Caster)
	{
		m_Caster->CalcDamage(*m_AttackData, m_Caster, OtherActor, Hit);
	}
	else
	{
		FLog::Log("No m_Caster");
	}
	
	DestroySelf();
}

void AElectricBall::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (m_Caster == OtherActor)
	{
		return;
	}
	
	DestroySelf();
}

void AElectricBall::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AElectricBall, StartPos);
	DOREPLIFETIME(AElectricBall, EndPos);
	DOREPLIFETIME(AElectricBall, Range);
	DOREPLIFETIME(AElectricBall, Speed);
	DOREPLIFETIME(AElectricBall, FlowTime);
	DOREPLIFETIME(AElectricBall, AttackTime);
}

