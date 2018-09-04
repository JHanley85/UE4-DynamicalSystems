// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef DYNSYS_SUBSYSTEM
#define DYNSYS_SUBSYSTEM FName(TEXT("DYNSYS"))
#endif
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemDynSysModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemDynSys.h"

IMPLEMENT_MODULE(FOnlineSubsystemDynSysModule, OnlineSubsystemDynSys);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryDynSys : public IOnlineFactory
{
public:

	FOnlineFactoryDynSys() {}
	virtual ~FOnlineFactoryDynSys() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemDynSysPtr OnlineSub = MakeShared<FOnlineSubsystemDynSys, ESPMode::ThreadSafe>(InstanceName);
		if (OnlineSub->IsEnabled())
		{
			if(!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("DynSys API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("DynSys API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemDynSysModule::StartupModule()
{
	DynSysFactory = new FOnlineFactoryDynSys();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(DYNSYS_SUBSYSTEM, DynSysFactory);
}

void FOnlineSubsystemDynSysModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(DYNSYS_SUBSYSTEM);
	
	delete DynSysFactory;
	DynSysFactory = NULL;
}
