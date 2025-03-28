// Fill out your copyright notice in the Description page of Project Settings.


#include "PalSphere.h"

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

void APalSphere::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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
	int randNum = FMath::RandRange(1,10);

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
