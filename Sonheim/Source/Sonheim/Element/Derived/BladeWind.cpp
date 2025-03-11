// Fill out your copyright notice in the Description page of Project Settings.


#include "BladeWind.h"

#include "Components/SphereComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values
ABladeWind::ABladeWind()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}
// Called when the game starts or when spawned
void ABladeWind::BeginPlay()
{
	Super::BeginPlay();

	Root->OnComponentBeginOverlap.AddDynamic(this, &ABladeWind::OnBeginOverlap);

}

void ABladeWind::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
	FAttackData* AttackData)
{
	Super::InitElement(Caster, Target, TargetLocation, AttackData);

	float ArcValue{FMath::RandRange(0.9f, 0.95f)};
	Root->AddImpulse(Fire(m_Caster, m_Target, m_TargetLocation, ArcValue));
}


// Called every frame
void ABladeWind::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABladeWind::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Super::OnBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

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

