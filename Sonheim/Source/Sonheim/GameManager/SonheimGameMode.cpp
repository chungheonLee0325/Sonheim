// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimGameMode.h"

#include "SonheimGameInstance.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ASonheimGameMode::ASonheimGameMode()
{
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BGMAudioComponennt"));
	AudioComponent->bAutoDestroy = false;
	AudioComponent->bAutoManageAttachment = false;
	SoundDataMap.Empty();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/AreaObject/Player/BP_Player.BP_Player_c'"));
	if (PlayerPawnBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	static ConstructorHelpers::FClassFinder<AController> PlayerControllerBPClass(TEXT(
		"/Script/Engine.Blueprint'/Game/_BluePrint/AreaObject/Player/BP_PlayerController.BP_PlayerController_c'"));
	if (PlayerControllerBPClass.Succeeded())
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
	static ConstructorHelpers::FClassFinder<APlayerState> PlayerStateBPClass(
		TEXT("/Script/Engine.Blueprint'/Game/_BluePrint/AreaObject/Player/BP_PlayerState.BP_PlayerState_c'"));
	if (PlayerStateBPClass.Succeeded())
	{
		PlayerStateClass = PlayerStateBPClass.Class;
	}
}

void ASonheimGameMode::BeginPlay()
{
	Super::BeginPlay();

	//PlayerDied 델리게이트를 게임 모드의 PlayerDied 함수에 바인딩.
	if (!OnPlayerDied.IsBound())
	{
		OnPlayerDied.AddDynamic(this, &ASonheimGameMode::PlayerDied);
	}
	auto gameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (nullptr == gameInstance)
	{
		return;
	}
	SoundDataMap = gameInstance->SoundDataMap;

	//PlayBGM(BGMID, true);
}


void ASonheimGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
}

void ASonheimGameMode::PlayerDied(ACharacter* Character)
{
	//캐릭터의 플레이어 컨트롤러에 대한 레퍼런스 구하기
	AController* CharacterController = Character->GetController();
	RestartPlayer(CharacterController);
}

void ASonheimGameMode::PlayGlobalSound(int SoundID)
{
	auto data = SoundDataMap.Find(SoundID);
	if (data != nullptr)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), *data);
	}
}

void ASonheimGameMode::PlayPositionalSound(int SoundID, FVector Position)
{
	auto data = SoundDataMap.Find(SoundID);
	if (data != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), *data, Position);
	}
}

void ASonheimGameMode::PlayBGM(int SoundID, bool bLoop)
{
	auto data = SoundDataMap.Find(SoundID);
	// 이전 BGM 정지
	StopBGM();

	// 새로운 BGM 설정
	if (data)
	{
		CurrentBGM = *data;
		AudioComponent->SetSound(CurrentBGM);
		AudioComponent->bAlwaysPlay = bLoop;
		AudioComponent->Play();
	}
}

void ASonheimGameMode::PlayBGMBySoundBase(USoundBase* SoundBase, bool bLoop)
{
	// 이전 BGM 정지
	StopBGM();

	// 새로운 BGM 설정
	if (SoundBase)
	{
		CurrentBGM = SoundBase;
		AudioComponent->SetSound(CurrentBGM);
		AudioComponent->bAlwaysPlay = bLoop;
		AudioComponent->Play();
	}
}

void ASonheimGameMode::StopBGM()
{
	// 현재 재생 중인 BGM 정지
	if (AudioComponent)
	{
		AudioComponent->Stop();
		CurrentBGM = nullptr;
	}
}

void ASonheimGameMode::SwitchBGMTemporary(int SoundID, float LifeTime)
{
	PreviousBGM = CurrentBGM;
	PlayBGM(SoundID, false);

	TWeakObjectPtr<ASonheimGameMode> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(SwitchBGMHandle, [WeakThis]()
	{
		if (auto StrongThis = WeakThis.Get())
		{
			StrongThis->PlayBGMBySoundBase(StrongThis->PreviousBGM, true);
		}
	}, LifeTime, false);
}

void ASonheimGameMode::SetBGMVolume(float Volume)
{
	// BGM 볼륨 조절 (0.0f ~ 1.0f)
	if (AudioComponent)
	{
		AudioComponent->SetVolumeMultiplier(FMath::Clamp(Volume, 0.0f, 1.0f));
	}
}
