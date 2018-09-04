// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemModule.h"
#include "OSSDynSysFL.generated.h"

/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API UOSSDynSysFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = Online)
		static bool SubsystemIsLoaded(FName Name) {
		FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
		OSS.GetOnlineSubsystem(Name);
		return OSS.IsOnlineSubsystemLoaded(Name);
	}
	UFUNCTION(BlueprintCallable)
		static void FindSessionFor(FName LvlName) {

	}
	
};
