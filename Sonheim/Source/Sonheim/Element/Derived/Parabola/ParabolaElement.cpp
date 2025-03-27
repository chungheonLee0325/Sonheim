// Fill out your copyright notice in the Description page of Project Settings.


#include "ParabolaElement.h"

#include "Components/SphereComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


AParabolaElement::AParabolaElement()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AParabolaElement::BeginPlay()
{
	Super::BeginPlay();
	Root->OnComponentBeginOverlap.AddDynamic(this, &AParabolaElement::OnBeginOverlap);
	Root->OnComponentHit.AddDynamic(this, &AParabolaElement::OnComponentHit);
}

void AParabolaElement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AParabolaElement::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
                                   FAttackData* AttackData)
{
	Super::InitElement(Caster, Target, TargetLocation, AttackData);

	float ArcValue{FMath::RandRange(0.8f, 0.9f)};
	Root->AddImpulse(Fire(m_Caster, m_Target, m_TargetLocation, ArcValue));
}

void AParabolaElement::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                      const FHitResult& SweepResult)
{
	if (m_Caster == OtherActor)
	{
		return;
	}
	
	FHitResult Hit;
	if (m_Caster)
	{
		if (m_AttackData != nullptr)
		{
			m_Caster->CalcDamage(*m_AttackData, m_Caster, OtherActor, Hit);
		}
		HandleBeginOverlap(m_Caster, OtherActor);
	}
	else
	{
		FLog::Log("No m_Caster");
	}
	
	DestroySelf();
}

void AParabolaElement::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (m_Caster == OtherActor)
	{
		return;
	}
	
	DestroySelf();
}
