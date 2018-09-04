// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"

class FOnlineSubsystemDynSys;

/** 
 * Implementation of session information
 */
class FOnlineSessionInfoDynSys : public FOnlineSessionInfo
{
protected:
	
	/** Hidden on purpose */
	FOnlineSessionInfoDynSys(const FOnlineSessionInfoDynSys& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoDynSys& operator=(const FOnlineSessionInfoDynSys& Src)
	{
		return *this;
	}

PACKAGE_SCOPE:

	/** Constructor */
	FOnlineSessionInfoDynSys();

	/** 
	 * Initialize a DynSys session info with the address of this machine
	 * and an id for the session
	 */
	void Init(const FOnlineSubsystemDynSys& Subsystem);

	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FUniqueNetIdString SessionId;

public:

	virtual ~FOnlineSessionInfoDynSys() {}

 	bool operator==(const FOnlineSessionInfoDynSys& Other) const
 	{
 		return false;
 	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		// LAN case
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"), 
			HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"), 
			*SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};
