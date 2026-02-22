// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Sonheim/Quest/QuestSnapshot.h"

#include "SonheimGameInstance.generated.h"

UCLASS()
class SONHEIM_API USonheimGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	static USonheimGameInstance* Get(class UWorld* World);

	// Session-only quest snapshot cache (server travel support)
	void SaveSessionQuestSnapshot(const FString& PlayerKey, const FQuestPlayerSnapshot& Snapshot);
	bool LoadSessionQuestSnapshot(const FString& PlayerKey, FQuestPlayerSnapshot& OutSnapshot) const;
	void ClearSessionQuestSnapshot(const FString& PlayerKey);

	UPROPERTY()
	TMap<FString, FQuestPlayerSnapshot> SessionQuestSnapshots;

	UPROPERTY(EditAnywhere)
	uint8 MaxPlayer{};

	UPROPERTY(EditAnywhere)
	FString RoomName{};

protected:
	virtual void Init() override;
};
