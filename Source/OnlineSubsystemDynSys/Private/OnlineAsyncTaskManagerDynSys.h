// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

/**
 *	DynSys version of the async task manager to register the various DynSys callbacks with the engine
 */
class FOnlineAsyncTaskManagerDynSys : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemDynSys* DynSysSubsystem;

public:

	FOnlineAsyncTaskManagerDynSys(class FOnlineSubsystemDynSys* InOnlineSubsystem)
		: DynSysSubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerDynSys() 
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};
