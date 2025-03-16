// Fill out your copyright notice in the Description page of Project Settings.


#include "ElectricWave.h"

#include "Engine/OverlapResult.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Utilities/LogMacro.h"

UElectricWave::UElectricWave()
{}

void UElectricWave::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);

	CurrentTime = 0.f;
	
	OnCastFire();
}

void UElectricWave::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);

	CurrentTime += DeltaTime;
	if (CurrentTime > DelayTime)
	{
		CurrentTime = 0.f;
		OnCastFire();
	}
}

void UElectricWave::OnCastFire()
{
	Super::OnCastFire();

	for (int32 i{}; i < AttackCount; ++i)
	{
		FTimerHandle ShockWaveTimer;
		GetWorld()->GetTimerManager().SetTimer(ShockWaveTimer, this, &UElectricWave::ShockWave, 0.2f * (i + 1), false);
	}
}

void UElectricWave::ShockWave()
{
	// sphere trace, 데미지 3번
	const FVector Start{m_Caster->GetActorLocation()};
	const FVector End{Start};

	const FCollisionShape CollisionShape{FCollisionShape::MakeSphere(Range)};
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(m_Caster);

	if (m_Caster->bShowDebug)
	{
		DrawDebugSphere(GetWorld(), Start, Range, 32, FColor::Red, false, 0.1f, 0, 1.0f);
		FLog::Log("ElectricWave::ShockWave");
	}
	
	TArray<FOverlapResult> OverlapResults;
	if (GetWorld()->OverlapMultiByChannel(OverlapResults, Start, FQuat::Identity, ECC_Visibility, CollisionShape,
										QueryParams))
	{
		for (FOverlapResult& Overlap : OverlapResults)
		{
			ASonheimPlayer* Player{Cast<ASonheimPlayer>(Overlap.GetActor())};
			if (Player)
			{
				FVector PlayerLocation{Player->GetActorLocation()};
				
				FHitResult CustomHitResult;
				CustomHitResult.Location = PlayerLocation;
				CustomHitResult.ImpactPoint = PlayerLocation;
				CustomHitResult.Normal = (PlayerLocation - Start).GetSafeNormal();
				
				if (m_Caster->bShowDebug)
				{
					LOG_PRINT(TEXT("%s"), *Player->GetName());
					DrawDebugSphere(GetWorld(), PlayerLocation, 5, 32, FColor::Green, false, 1.0f, 0, 1.0f);
				}
				
				FAttackData* AttackData = GetAttackDataByIndex(0);
				m_Caster->CalcDamage(*AttackData, m_Caster, Player, CustomHitResult);	
			}
		}
	}
}
