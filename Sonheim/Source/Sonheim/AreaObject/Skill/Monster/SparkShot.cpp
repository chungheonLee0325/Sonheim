// Fill out your copyright notice in the Description page of Project Settings.


#include "SparkShot.h"

#include "MaterialHLSLTree.h"
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

void USparkShot::Server_OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	IsFired = false;

	CurrentTime = 0.f;

	Super::Server_OnCastStart(Caster, Target);

	//OnCastFire();
}

void USparkShot::Server_OnCastTick(float DeltaTime)
{
	Super::Server_OnCastTick(DeltaTime);

	// CurrentTime += DeltaTime;
	// if (CurrentTime > DelayTime)
	// {
	// 	CurrentTime = 0.f;
	// 	OnCastFire();
	// }
}

void USparkShot::Server_OnCastFire()
{
	if (IsFired) return;

	Super::Server_OnCastFire();

	FireSparkShot();
}

void USparkShot::FireSparkShot()
{
	IsFired = true;

	float StartAngle{-30.f};
	float AngleIncrease{10.f};

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

		//m_Target = TempTarget;

		float Angle{StartAngle + AngleIncrease * i};
		FVector Direction{
			FVector(FMath::Cos(FMath::DegreesToRadians(Angle)),
			        FMath::Sin(FMath::DegreesToRadians(Angle)),
			        0.f)
		};

		// 캐릭터 회전 고려해서 회전
		FRotator Rotation{m_Caster->GetActorRotation()};
		Direction = Rotation.RotateVector(Direction);
		Direction.Normalize();

		m_TargetPos = Direction;

		if (SpawnedLightningBall)
		{
			SpawnedLightningBall->InitElement(m_Caster, m_Target, m_TargetPos, AttackData);
		}
	}
}
