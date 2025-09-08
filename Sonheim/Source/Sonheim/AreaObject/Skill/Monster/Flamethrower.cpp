// Fill out your copyright notice in the Description page of Project Settings.


#include "Flamethrower.h"

#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "DrawDebugHelpers.h"


UFlamethrower::UFlamethrower()
{
}

bool UFlamethrower::Activate(class AAreaObject* Caster, AAreaObject* Target)
{
	if (!Super::Activate(Caster, Target)) return false;

	return true;
}

void UFlamethrower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool UFlamethrower::Fire()
{
	if(!Super::Fire()) return false;
	
	FireFlame();
	m_CurrentPhase = ESkillPhase::Casting;

	return true;
}

void UFlamethrower::FireFlame()
{
	ASonheimPlayer* PartnerOwner = Cast<ABaseMonster>(m_Caster)->PartnerOwner;

	FVector StartPos{m_Caster->GetMesh()->GetSocketLocation(FName("Socket_Mouth"))};
	// 에임 방향: 록온이면 카메라 중심 방향, 아니면 팔(캐스터) 정면 방향
	FVector AimDir = m_Caster->GetActorForwardVector();
	if (PartnerOwner)
	{
		if (PartnerOwner->IsLockOn())
		{
			AimDir = PartnerOwner->GetFollowCamera()->GetForwardVector();
		}
		else
		{
			AimDir = m_Caster->GetActorForwardVector();
		}
	}
	FVector EndPos{
		StartPos + UKismetMathLibrary::RandomUnitVectorInEllipticalConeInDegrees(
			AimDir, SpreadYaw, SpreadPitch) * Range
	};

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(m_Caster);
	TArray<FHitResult> HitInfos;

	// 파트너 소유자(플레이어)와 캐스터는 모두 무시
	Params.AddIgnoredActor(PartnerOwner);

	// 스피어 스윕으로 판정 (화염 방사 커버리지 향상)
	FAttackData* AttackDataForSweep = GetAttackDataByIndex(0);
	const float SphereRadius = (AttackDataForSweep && AttackDataForSweep->HitBoxData.Radius > 0.f)
		                           ? AttackDataForSweep->HitBoxData.Radius
		                           : 15.f;

	bool bHit{
		GetWorld()->SweepMultiByChannel(
			HitInfos,
			StartPos,
			EndPos,
			FQuat::Identity,
			ECollisionChannel::ECC_GameTraceChannel7,
			FCollisionShape::MakeSphere(SphereRadius),
			Params)
	};

	if (m_Caster->bShowDebug)
	{
		// 메인 경로 표시
		DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Red, false, 0.2f, 0, 1.f);
		// 스윕 반경 가시화: 경로를 따라 일정 간격으로 스피어를 그림
		const int32 Steps = 6;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const float T = static_cast<float>(i) / Steps;
			const FVector P = FMath::Lerp(StartPos, EndPos, T);
			DrawDebugSphere(GetWorld(), P, SphereRadius, 8, FColor::Orange, false, 0.2f, 0, 1.f);
		}
	}

	if (bHit)
	{
		TSet<AActor*> UniqueActors;
		for (FHitResult& HitInfo : HitInfos)
		{
			if (AActor* HitActor = HitInfo.GetActor())
			{
				if (!UniqueActors.Contains(HitActor))
				{
					UniqueActors.Add(HitActor);
					if (m_Caster->bShowDebug)
					{
						DrawDebugSphere(GetWorld(), HitInfo.ImpactPoint, SphereRadius * 0.6f, 12, FColor::Yellow, false,
						                0.3f, 0, 1.f);
					}
					FAttackData* AttackData = GetAttackDataByIndex(0);
					if (AttackData)
					{
						m_Caster->CalcDamage(*AttackData, PartnerOwner, HitActor, HitInfo);
					}
				}
			}
		}
	}
}
