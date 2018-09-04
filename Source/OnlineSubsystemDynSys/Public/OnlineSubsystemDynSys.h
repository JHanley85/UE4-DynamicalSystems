// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemDynSysPackage.h"
#include "HAL/ThreadSafeCounter.h"

class FOnlineAchievementsDynSys;
class FOnlineIdentityDynSys;
class FOnlineLeaderboardsDynSys;
class FOnlineSessionDynSys;
class FOnlineVoiceImpl;

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionDynSys, ESPMode::ThreadSafe> FOnlineSessionDynSysPtr;
typedef TSharedPtr<class FOnlineProfileDynSys, ESPMode::ThreadSafe> FOnlineProfileDynSysPtr;
typedef TSharedPtr<class FOnlineFriendsDynSys, ESPMode::ThreadSafe> FOnlineFriendsDynSysPtr;
typedef TSharedPtr<class FOnlineUserCloudDynSys, ESPMode::ThreadSafe> FOnlineUserCloudDynSysPtr;
typedef TSharedPtr<class FOnlineLeaderboardsDynSys, ESPMode::ThreadSafe> FOnlineLeaderboardsDynSysPtr;
typedef TSharedPtr<class FOnlineVoiceImpl, ESPMode::ThreadSafe> FOnlineVoiceImplPtr;
typedef TSharedPtr<class FOnlineExternalUIDynSys, ESPMode::ThreadSafe> FOnlineExternalUIDynSysPtr;
typedef TSharedPtr<class FOnlineIdentityDynSys, ESPMode::ThreadSafe> FOnlineIdentityDynSysPtr;
typedef TSharedPtr<class FOnlineAchievementsDynSys, ESPMode::ThreadSafe> FOnlineAchievementsDynSysPtr;

/**
 *	OnlineSubsystemDynSys - Implementation of the online subsystem for DynSys services
 */
class ONLINESUBSYSTEMDYNSYS_API FOnlineSubsystemDynSys : 
	public FOnlineSubsystemImpl
{

public:

	virtual ~FOnlineSubsystemDynSys()
	{
	}
	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;	
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStorePtr GetStoreInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	
	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;

	// FTickerObjectBase
	
	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemDynSys

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemDynSys(FName InInstanceName) :
		FOnlineSubsystemImpl(NULL_SUBSYSTEM, InInstanceName),
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

	FOnlineSubsystemDynSys() :
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

private:

	/** Interface to the session services */
	FOnlineSessionDynSysPtr SessionInterface;

	/** Interface for voice communication */
	mutable FOnlineVoiceImplPtr VoiceInterface;

	/** Interface for voice communication */
	mutable bool bVoiceInterfaceInitialized;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsDynSysPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityDynSysPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsDynSysPtr AchievementsInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerDynSys* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	// task counter, used to generate unique thread names for each task
	static FThreadSafeCounter TaskCounter;
};

typedef TSharedPtr<FOnlineSubsystemDynSys, ESPMode::ThreadSafe> FOnlineSubsystemDynSysPtr;

