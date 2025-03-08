// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SonheimGameMode.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDiedSignature, ACharacter*, Character);

/**
 * 
 */
UCLASS()
class SONHEIM_API ASonheimGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASonheimGameMode();

	const FOnPlayerDiedSignature& GetOnPlayerDied() const { return OnPlayerDied; }

	//플레이어의 폰을 스폰 시도.
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayGlobalSound(int SoundID);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayPositionalSound(int SoundID, FVector Position);

	// BGM 재생 함수
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBGM(int SoundID, bool bLoop = true);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBGMBySoundBase(USoundBase* SoundBase, bool bLoop = true);

	// BGM 정지 함수
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBGM();

	// BGM 잠시 변경 재생 함수
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SwitchBGMTemporary(int SoundID, float LifeTime);

	// BGM 볼륨 조절 함수
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetBGMVolume(float Volume);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SoundManager")
	TMap<int, USoundBase*> SoundDataMap;

protected:
	virtual void BeginPlay() override;

	//플레이어 캐릭터가 죽으면 호출.
	UFUNCTION()
	virtual void PlayerDied(ACharacter* Character);
	void PlayBGM(int SoundID);

	//델리게이트를 바인딩할 시그니처.
	UPROPERTY()
	FOnPlayerDiedSignature OnPlayerDied;

private:
	// 오디오 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* AudioComponent;

	// 현재 재생 중인 BGM
	UPROPERTY()
	USoundBase* CurrentBGM;

	UPROPERTY()
	USoundBase* PreviousBGM;

	UPROPERTY()
	FTimerHandle SwitchBGMHandle;
};
