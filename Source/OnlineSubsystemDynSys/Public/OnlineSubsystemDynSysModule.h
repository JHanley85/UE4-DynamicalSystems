// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Online subsystem module class  (DynSys Implementation)
 * Code related to the loading of the DynSys module
 */
class FOnlineSubsystemDynSysModule : public IModuleInterface
{
private:

	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryDynSys* DynSysFactory;

public:

	FOnlineSubsystemDynSysModule() : 
		DynSysFactory(NULL)
	{}

	virtual ~FOnlineSubsystemDynSysModule() {}

	// IModuleInterface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}
};