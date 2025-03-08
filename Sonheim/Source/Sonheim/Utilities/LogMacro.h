#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(SONHEIM, Log, All);

#define CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))
#define PRINT_CALLINFO() \
UE_LOG(SONHEIM, Warning, TEXT("%s"), *CALLINFO)

#define LOG_PRINT(fmt, ...) \
UE_LOG(SONHEIM, Warning, TEXT("%s %s"), *CALLINFO, *FString::Printf(fmt, ##__VA_ARGS__))

#define LOG_SCREEN_MY(Time, Color, Format, ...) \
if (GEngine) GEngine->AddOnScreenDebugMessage(-1, Time, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__));

#define LOG_SCREEN(Format, ...) \
LOG_SCREEN_MY(2.0f, FColor::Green, Format,  ##__VA_ARGS__)

#define LOG_SCREEN_ERROR(Obj, Format, ...) \
if (GEngine && Obj) \
{ \
FString Msg = FString::Printf(TEXT("[%s] " Format), *Obj->GetName(), ##__VA_ARGS__); \
GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, Msg); \
}

#define LOG_SCREEN_OBJ(Obj, Format, ...) \
if (GEngine && Obj) \
{ \
FString Msg = FString::Printf(TEXT("[%s] " Format), *Obj->GetName(), ##__VA_ARGS__); \
GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, Msg); \
}