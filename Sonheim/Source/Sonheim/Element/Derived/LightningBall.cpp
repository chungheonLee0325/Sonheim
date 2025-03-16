// Fill out your copyright notice in the Description page of Project Settings.


#include "LightningBall.h"

#include "Components/SphereComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values
ALightningBall::ALightningBall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root->SetSimulatePhysics(false);

	Root->SetSphereRadius(15.f);
	Mesh->SetRelativeScale3D(FVector(0.3f));
}

// Called when the game starts or when spawned
void ALightningBall::BeginPlay()
{
	Super::BeginPlay();
	
	Root->OnComponentBeginOverlap.AddDynamic(this, &ALightningBall::OnBeginOverlap);

}

// Called every frame
void ALightningBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetActorLocation(GetActorLocation() + DeltaTime * m_TargetLocation * Speed);
	
	if (FVector::Dist(StartPos, GetActorLocation()) > Range)
	{
		//FLog::Log("Dist", FVector::Dist(StartPos, GetActorLocation()));
		DestroySelf();
	}
}

void ALightningBall::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
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

	//FLog::Log("count");

}

void ALightningBall::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

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

