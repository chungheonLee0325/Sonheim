// Fill out your copyright notice in the Description page of Project Settings.

#include "SonheimGameInstance.h"

#include "Sonheim/Utilities/SessionUtil.h"

void USonheimGameInstance::Init()
{
	Super::Init();
	FSessionUtil::Init();
}

USonheimGameInstance* USonheimGameInstance::Get(class UWorld* World)
{
	return World ? Cast<USonheimGameInstance>(World->GetGameInstance()) : nullptr;
}

void USonheimGameInstance::SaveSessionQuestSnapshot(const FString& PlayerKey, const FQuestPlayerSnapshot& Snapshot)
{
	if (PlayerKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip saving quest snapshot: empty player key."));
		return;
	}

	SessionQuestSnapshots.Add(PlayerKey, Snapshot);
}

bool USonheimGameInstance::LoadSessionQuestSnapshot(const FString& PlayerKey, FQuestPlayerSnapshot& OutSnapshot) const
{
	if (PlayerKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip loading quest snapshot: empty player key."));
		return false;
	}

	if (const FQuestPlayerSnapshot* Found = SessionQuestSnapshots.Find(PlayerKey))
	{
		OutSnapshot = *Found;
		return true;
	}

	return false;
}

void USonheimGameInstance::ClearSessionQuestSnapshot(const FString& PlayerKey)
{
	if (PlayerKey.IsEmpty())
	{
		return;
	}

	SessionQuestSnapshots.Remove(PlayerKey);
}
