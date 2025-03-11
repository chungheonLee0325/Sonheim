// Fill out your copyright notice in the Description page of Project Settings.


#include "SandBlast.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values
ASandBlast::ASandBlast()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void ASandBlast::BeginPlay()
{
	Super::BeginPlay();
	Root->OnComponentBeginOverlap.AddDynamic(this, &ASandBlast::OnBeginOverlap);
	
	// FLog::Log("ASandBlast::BeginPlay");
}

void ASandBlast::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation, FAttackData* AttackData)
{
	Super::InitElement(Caster, Target, TargetLocation, AttackData);

	float ArcValue{FMath::RandRange(0.5f, 0.6f)};
	Root->AddImpulse(Fire(m_Caster, m_Target, m_TargetLocation, ArcValue));
}

// Called every frame
void ASandBlast::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ASandBlast::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
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
