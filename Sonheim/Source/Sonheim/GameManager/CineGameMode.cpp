// Fill out your copyright notice in the Description page of Project Settings.


#include "CineGameMode.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "SonheimGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/Utilities/SessionUtil.h"

void ACineGameMode::BeginPlay()
{
	Super::BeginPlay();

	ULevelSequence* MySequence{
		LoadObject<ULevelSequence>(nullptr,
		                           TEXT("/Game/_Resource/Cine/Sequences/TotalTake.TotalTake"))
	};
	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* LevelSequenceActor;
	ULevelSequencePlayer* SequencePlayer{
		ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), MySequence, PlaybackSettings, LevelSequenceActor)
	};

	if (SequencePlayer && LevelSequenceActor)
	{
		SequencePlayer->OnFinished.AddDynamic(this, &ACineGameMode::OnSequenceFinished);
		SequencePlayer->Play();
	}
}

void ACineGameMode::OnSequenceFinished()
{
	auto Camera{UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)};
	if (Camera)
	{
		Camera->SetManualCameraFade(1.f, FColor::Black, false);
	}
	
	auto GS{Cast<USonheimGameInstance>(GetGameInstance())};

	FSessionCreateData CreateData;
	CreateData.IsPublic = true;
	CreateData.MaxPlayer = GS->MaxPlayer;
	CreateData.RoomName = GS->RoomName;
	
	FSessionUtil::CreateSession(CreateData);
	
	GetWorld()->ServerTravel(FString("/Game/_Maps/GameMap?listen"));
}
