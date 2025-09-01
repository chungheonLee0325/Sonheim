// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "SonheimUtility.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USonheimUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static float CalculateDamageMultiplier(EElementalAttribute DefenseAttribute, EElementalAttribute AttackAttribute);

	UFUNCTION(BlueprintPure, Category = "Utility|Item")
	static FLinearColor GetRarityColor(EItemRarity Rarity, float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Utility|Item")
	static FText ConvertRarityText(EItemRarity Rarity);

	static bool CheckMoveEnable(const UObject* WorldContextObject, const class AAreaObject* Caster, const class AAreaObject* Target, const FVector& StartLoc, FVector& EndLoc);

	// 개행 문자 처리를 위한 헬퍼 메서드
	static FText ConvertEscapedNewlinesToFText(const FText& InputText);

	/** 
	 * 아이템 액터를 Count개 스폰. 1 actor = Options.ItemCount 개 아이템.
	 * 산포는 Origin을 기준으로 ScatterRadius 안에서 무작위 배치.
	 * 서버 권한에서만 수행.
	 */
	UFUNCTION(BlueprintCallable, Category="Utility|Item", meta=(WorldContext="WorldContextObject"))
	static bool SpawnItems(const UObject* WorldContextObject, int32 ItemID, int32 Count,
	                       const FVector& Origin, float ScatterRadius,
	                       const FItemSpawnOptions& Options);

	/**
	 * 확률 테이블 기반 드랍(세그먼트 수만큼 반복). 
	 * 테이블 키: ItemID, 값: 드랍 확률(1~100).
	 * 각 적중 시 SpawnItems(..., Count=1, ...) 호출.
	 * 서버 권한에서만 수행.
	 */
	UFUNCTION(BlueprintCallable, Category="Utility|Item", meta=(WorldContext="WorldContextObject"))
	static void SpawnFromDropTableWithOptions(const UObject* WorldContextObject,
	                                          const TMap<int32, int32>& DropChance,
	                                          const FVector& BaseLoc, int32 Segments,
	                                          float ScatterRadius,
	                                          const FItemSpawnOptions& Options);
};
