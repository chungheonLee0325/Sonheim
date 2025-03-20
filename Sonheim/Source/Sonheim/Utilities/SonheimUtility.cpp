// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimUtility.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"


float USonheimUtility::CalculateDamageMultiplier(EElementalAttribute DefenseAttribute,
                                                 EElementalAttribute AttackAttribute)
{
	// 속성이 None인 경우 기본 배율 1.0 반환
	if (DefenseAttribute == EElementalAttribute::None || AttackAttribute == EElementalAttribute::None)
	{
		return 1.0f;
	}

	// 데미지 배율 테이블 정의
	// [방어 속성][공격 속성]
	static const float DamageMultiplierTable[9][9] = {
		//   Grass   Fire  Water Electric Ground  Ice   Dragon  Dark  Neutral
		{  1.0f,  2.0f,  1.0f,  1.0f,   0.5f,  1.0f,  1.0f,  1.0f,  1.0f  }, // Grass
		{  0.5f,  1.0f,  2.0f,  1.0f,   1.0f,  0.5f,  1.0f,  1.0f,  1.0f  }, // Fire
		{  1.0f,  0.5f,  1.0f,  2.0f,   1.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // Water
		{  1.0f,  1.0f,  0.5f,  1.0f,   2.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // Electric
		{  2.0f,  1.0f,  1.0f,  0.5f,   1.0f,  1.0f,  1.0f,  1.0f,  1.0f  }, // Ground
		{  1.0f,  2.0f,  1.0f,  1.0f,   1.0f,  1.0f,  0.5f,  1.0f,  1.0f  }, // Ice
		{  1.0f,  1.0f,  1.0f,  1.0f,   1.0f,  2.0f,  1.0f,  0.5f,  1.0f  }, // Dragon
		{  1.0f,  1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  2.0f,  1.0f,  0.5f  }, // Dark
		{  1.0f,  1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,  2.0f,  1.0f  }  // Neutral
	};

	// 열거형 값을 인덱스로 변환 (None을 제외하고 시작하므로 -1)
	int defenseIndex = static_cast<int>(DefenseAttribute) - 1;
	int attackIndex = static_cast<int>(AttackAttribute) - 1;

	// 유효한 인덱스 범위 확인
	if (defenseIndex >= 0 && defenseIndex < 9 && attackIndex >= 0 && attackIndex < 9)
	{
		return DamageMultiplierTable[defenseIndex][attackIndex];
	}

	// 잘못된 속성 값이 들어온 경우 기본 배율 1.0 반환
	return 1.0f;
}

bool USonheimUtility::CheckMoveEnable(const UObject* WorldContextObject, const class AAreaObject* Caster, const class AAreaObject* Target, const FVector& StartLoc, const FVector& EndLoc)
{
	UWorld* World{GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)};
	if (!World)
	{
		return false;
	}
	
	// StartLoc에서 EndLoc까지 중심점
	const FVector TraceCenter{(StartLoc + EndLoc) * 0.5f};

	// StartLoc과 EndLoc 방향
	const FVector Direction{(EndLoc - StartLoc).GetSafeNormal()};
	// StartLoc과 EndLoc 거리 계산
	const float Distance{static_cast<float>(FVector::Distance(StartLoc, EndLoc))};

	// 박스의 크기
	const FVector TraceExtent{FVector(Distance * 0.5f, 50.f, 5.f)};
	// 방향 벡터로 회전 계산
	const FQuat BoxRotation{FRotationMatrix::MakeFromX(Direction).ToQuat()};

	// BoxTrace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Caster);
	QueryParams.AddIgnoredActor(Target);
	const FCollisionShape CollisionShape{FCollisionShape::MakeBox(TraceExtent)};
	const bool bHit{
		World->SweepSingleByChannel(
			HitResult,
			TraceCenter,
			TraceCenter,
			BoxRotation,
			ECC_Visibility,
			CollisionShape,
			QueryParams
		)
	};

	// 디버그용으로 BoxTrace 시각화
	if (Caster->bShowDebug)
	{
		if (bHit)
		{
			DrawDebugBox(World, TraceCenter, TraceExtent, BoxRotation, FColor::Red, false, 2.f);
		}
		else
		{
			DrawDebugBox(World, TraceCenter, TraceExtent, BoxRotation, FColor::Green, false, 2.f);
		}
	}

	// hit 없으면 공격 가능
	return !bHit;
}

FText USonheimUtility::ConvertEscapedNewlinesToFText(const FText& InputText)
{
	// FText -> FString로 변환
	FString OriginalString = InputText.ToString();

	// \r\n이나 \n 같은 개행 문자열을 실제 개행 문자로 변환
	OriginalString = OriginalString.Replace(TEXT("\\r\\n"), TEXT("\n"));
	OriginalString = OriginalString.Replace(TEXT("\\n"), TEXT("\n"));
	OriginalString = OriginalString.Replace(TEXT("\\r"), TEXT("\r"));
    
	// 변환된 문자열을 다시 FText로 변환하여 반환
	return FText::FromString(OriginalString);
}
