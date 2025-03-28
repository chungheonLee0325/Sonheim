#pragma once

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Interfaces/OnlineSessionInterface.h"

class FOnlineSessionSearch;

struct FSessionCreateData
{
	uint8 MaxPlayer;
	bool IsPublic = true;
	FString RoomName;
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
};

struct FSessionSearchData
{
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
};

class SONHEIM_API FSessionUtil
{
	
public:
	static IOnlineSessionPtr OnlineSessionInterface;
	
	static void Init();
	static void CreateSession(const FSessionCreateData& SessionCreateData);
	static void SearchSession(FSessionSearchData& SessionSearchData);
	static void JoinSession(const UWorld* World, FOnlineSessionSearchResult& Result,
		const FOnJoinSessionCompleteDelegate& OnJoinSessionCompleteDelegate);
	static void DestroySession();
	
	static FNamedOnlineSession* GetCurrentSession();
	static FString EncodeData(const FString& Str);
	static FString DecodeData(const FString& Str);
	
private:
	static FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	static FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	static FDelegateHandle OnJoinSessionCompleteDelegateHandle;
};