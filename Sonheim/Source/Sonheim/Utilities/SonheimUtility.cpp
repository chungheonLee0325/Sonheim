// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimUtility.h"



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
