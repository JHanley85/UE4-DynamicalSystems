// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemDynSysTypes.h"
#include "NboSerializer.h"

/**
 * Serializes data in network byte order form into a buffer
 */
class FNboSerializeToBufferDynSys : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferDynSys() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferDynSys(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	 * Adds DynSys session info to the buffer
	 */
 	friend inline FNboSerializeToBufferDynSys& operator<<(FNboSerializeToBufferDynSys& Ar, const FOnlineSessionInfoDynSys& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Adds DynSys Unique Id to the buffer
	 */
	friend inline FNboSerializeToBufferDynSys& operator<<(FNboSerializeToBufferDynSys& Ar, const FUniqueNetIdString& UniqueId)
	{
		Ar << UniqueId.UniqueNetIdStr;
		return Ar;
	}
};

/**
 * Class used to write data into packets for sending via system link
 */
class FNboSerializeFromBufferDynSys : public FNboSerializeFromBuffer
{
public:
	/**
	 * Initializes the buffer, size, and zeros the read offset
	 */
	FNboSerializeFromBufferDynSys(uint8* Packet,int32 Length) :
		FNboSerializeFromBuffer(Packet,Length)
	{
	}

	/**
	 * Reads DynSys session info from the buffer
	 */
 	friend inline FNboSerializeFromBufferDynSys& operator>>(FNboSerializeFromBufferDynSys& Ar, FOnlineSessionInfoDynSys& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId; 
		Ar >> *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Reads DynSys Unique Id from the buffer
	 */
	friend inline FNboSerializeFromBufferDynSys& operator>>(FNboSerializeFromBufferDynSys& Ar, FUniqueNetIdString& UniqueId)
	{
		Ar >> UniqueId.UniqueNetIdStr;
		return Ar;
	}
};
