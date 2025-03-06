// Copyright Epic Games, Inc. All Rights Reserved.

#include "SonheimGameMode.h"
#include "SonheimCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASonheimGameMode::ASonheimGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
