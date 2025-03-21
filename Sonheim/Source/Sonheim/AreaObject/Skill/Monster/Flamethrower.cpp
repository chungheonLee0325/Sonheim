// Fill out your copyright notice in the Description page of Project Settings.


#include "Flamethrower.h"

#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Utilities/LogMacro.h"


UFlamethrower::UFlamethrower()
{}

void UFlamethrower::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	Super::OnCastStart(Caster, Target);

}

void UFlamethrower::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);
}

void UFlamethrower::OnCastFire()
{
	Super::OnCastFire();
	
	//GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &UFlamethrower::FireFlame, 0.2f, true);
	FireFlame();
}

void UFlamethrower::FireFlame()
{
	FLog::Log();
	
	// FVector StartPos{m_Caster->GetActorLocation()};
	// FVector EndPos{
	// 	StartPos + UKismetMathLibrary::RandomUnitVectorInEllipticalConeInDegrees(
	// 		m_Caster->GetActorForwardVector(), SpreadYaw, SpreadPitch) * Range
	// };

	ASonheimPlayer* PartnerOwner = {Cast<ABaseMonster>(m_Caster)->PartnerOwner};

	// ToDo : PartnerOwner 설정되면 없애기
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	PartnerOwner = Player;
	
	FVector StartPos{m_Caster->GetMesh()->GetSocketLocation(FName("Socket_Mouth"))};
	FVector EndPos{
		StartPos + UKismetMathLibrary::RandomUnitVectorInEllipticalConeInDegrees(
			PartnerOwner->GetFollowCamera()->GetForwardVector(), SpreadYaw, SpreadPitch) * Range
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(m_Caster);
	TArray<FHitResult> HitInfos;

	// ECC_GameTraceChannel7 : Flamethrower
	bool bHit{
		GetWorld()->LineTraceMultiByChannel(HitInfos, StartPos, EndPos, ECollisionChannel::ECC_GameTraceChannel7,
		                                    Params)
	};

	if (m_Caster->bShowDebug)
	{
		DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Red, false, 1.f, 0, 1.f);
	}

	for (FHitResult& HitInfo : HitInfos)
	{
		if (m_Caster->bShowDebug)
		{
			//DrawDebugLine(GetWorld(), StartPos, HitInfo.ImpactPoint, FColor::Red, false, 1.f, 0, 1.f);
		}

		FAttackData* AttackData = GetAttackDataByIndex(0);
		m_Caster->CalcDamage(*AttackData, PartnerOwner, HitInfo.GetActor(), HitInfo);
		
		// 
		// ASonheimPlayer* Player{Cast<ASonheimPlayer>(HitInfo.GetActor())};
		// if (Player)
		// {
		// 	FAttackData* AttackData = GetAttackDataByIndex(0);
		// 	m_Caster->CalcDamage(*AttackData, m_Caster, Player, HitInfo);
		// }
	}
}
