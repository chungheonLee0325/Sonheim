// Fill out your copyright notice in the Description page of Project Settings.


#include "SparkShot.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Element/Derived/LightningBall.h"
#include "Sonheim/Utilities/LogMacro.h"


USparkShot::USparkShot()
{
	static ConstructorHelpers::FClassFinder<ALightningBall> LightningBallClass
		(TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/Element/LightningBall/BP_LightningBall.BP_LightningBall_C'"));
	if (LightningBallClass.Succeeded())
	{
		LightingBallFactory = LightningBallClass.Class;
	}
}

void USparkShot::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	CurrentTime = 0.f;

	Super::OnCastStart(Caster, Target);

	OnCastFire();
}

void USparkShot::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	CurrentTime += DeltaTime;
	if (CurrentTime > DelayTime)
	{
		CurrentTime = 0.f;
		OnCastFire();
	}
}

void USparkShot::OnCastFire()
{
	Super::OnCastFire();

	FireSparkShot();
}

void USparkShot::FireSparkShot()
{
	for (int32 i{}; i < AttackCount; ++i)
	{
		ALightningBall* SpawnedLightningBall{
			GetWorld()->SpawnActor<ALightningBall>(LightingBallFactory, m_Caster->GetActorLocation(),
			                                       m_Caster->GetActorRotation())
		};

		// ToDo : Notify에서 Index 주입
		FAttackData* AttackData = GetAttackDataByIndex(0);
		// ToDo : TempTarget -> m_Target으로 수정
		ASonheimPlayer* TempTarget{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};

		m_Target = TempTarget;
		m_TargetPos = m_Caster->GetActorLocation() + 
				FVector(FMath::Cos(FMath::DegreesToRadians(-30.f + 10 * i)),
					FMath::Sin(FMath::DegreesToRadians(-30.f + 10 * i )),
					m_Target->GetActorLocation().Z) * 500.f;
		m_TargetPos.Normalize();
		
		if (SpawnedLightningBall)
		{
			SpawnedLightningBall->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
		}
	}
}
