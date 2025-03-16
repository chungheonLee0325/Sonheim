// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "LevelComponent.generated.h"

class AAreaObject;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLevelChanged, int32, OldLevel, int32, NewLevel, bool, bLevelUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExperienceChanged, int32, CurrentExp, int32, MaxExp, int32, Delta);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API ULevelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULevelComponent();

    UFUNCTION(BlueprintCallable, Category = "Level")
	void InitLevel(class AAreaObject* Parent);
	
    // 레벨 업 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Level")
    FOnLevelChanged OnLevelChanged;

    // 경험치 변경 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Level")
    FOnExperienceChanged OnExperienceChanged;

    // 경험치 획득
    UFUNCTION(BlueprintCallable, Category = "Level")
    void AddExp(int32 ExpAmount);

    // 레벨 설정
    UFUNCTION(BlueprintCallable, Category = "Level")
    void SetLevel(int32 NewLevel);

    // 경험치 설정
    UFUNCTION(BlueprintCallable, Category = "Level")
    void SetExp(int32 NewExp);

    // 현재 레벨 반환
    UFUNCTION(BlueprintCallable, Category = "Level")
    int32 GetCurrentLevel() const { return CurrentLevel; }

    // 현재 경험치 반환
    UFUNCTION(BlueprintCallable, Category = "Level")
    float GetCurrentExp() const { return CurrentExp; }

    // 다음 레벨까지 필요 경험치 반환
    UFUNCTION(BlueprintCallable, Category = "Level")
    int32 GetExpToNextLevel() const { return ExpToNextLevel; }

    // 스탯 포인트 획득
    UFUNCTION(BlueprintCallable, Category = "Level")
    int32 GetAvailableStatPoints() const { return AvailableStatPoints; }

    // 스탯 포인트 사용
    UFUNCTION(BlueprintCallable, Category = "Level")
    void UseStatPoints(int32 Points);

    // 레벨에 따른 경험치 계산
    UFUNCTION(BlueprintCallable, Category = "Level")
    int32 GetExpForLevel(int32 Level);
protected:
    virtual void BeginPlay() override;

    // 레벨업 처리
    void HandleLevelUp();

    // 레벨에 따른 스탯 증가
    void ApplyLevelUpBonuses(int32 OldLevel, int32 NewLevel);

protected:
    // 현재 레벨
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 CurrentLevel;

    // 현재 경험치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 CurrentExp;

    // 다음 레벨까지 필요 경험치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 ExpToNextLevel;

    // 최대 레벨
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 MaxLevel;

    // 사용 가능한 스탯 포인트
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 AvailableStatPoints;

    // 레벨 업 당 스탯 포인트 획득량
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    int32 StatPointsPerLevel;
	
private:
	TMap<int32, FLevelData>* dt_Level;
	UPROPERTY()
	AAreaObject* m_Owner;
}; 