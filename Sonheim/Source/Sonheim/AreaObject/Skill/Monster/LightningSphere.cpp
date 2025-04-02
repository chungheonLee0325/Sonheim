// Fill out your copyright notice in the Description page of Project Settings.


#include "LightningSphere.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Element/Derived/ElectricBall.h"

ULightningSphere::ULightningSphere()
{
	static ConstructorHelpers::FClassFinder<AElectricBall> ElectricBallClass
	(TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/Element/ElectricBall/BP_ElectricBall.BP_ElectricBall_C'"));
	if (ElectricBallClass.Succeeded())
	{
		ElectricBallFactory = ElectricBallClass.Class;
	}
}

void ULightningSphere::Server_OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	IsFired = false;

	Super::Server_OnCastStart(Caster, Target);
	
	CurrentTime = 0.f;
	
}

void ULightningSphere::Server_OnCastTick(float DeltaTime)
{
	Super::Server_OnCastTick(DeltaTime);

	// CurrentTime += DeltaTime;
	// if (CurrentTime > DelayTime)
	// {
	// 	CurrentTime = 0.f;
	// 	OnCastFire();
	// }
}

void ULightningSphere::Server_OnCastFire()
{
	if (IsFired) return;

	Super::Server_OnCastFire();

	FireElectricBall();
}

void ULightningSphere::FireElectricBall()
{
	IsFired = true;

	AElectricBall* SpawnedElectricBall{
		GetWorld()->SpawnActor<AElectricBall>(ElectricBallFactory, m_Caster->GetActorLocation(),
											   m_Caster->GetActorRotation())
	};

	// ToDo : Notify에서 Index 주입
	FAttackData* AttackData = GetAttackDataByIndex(0);
	// ToDo : TempTarget -> m_Target으로 수정
	ASonheimPlayer* TempTarget{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};

	//m_Target = TempTarget;
	
	m_TargetPos = m_Caster->GetActorForwardVector();

	if (SpawnedElectricBall)
	{
		SpawnedElectricBall->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
	}
}

