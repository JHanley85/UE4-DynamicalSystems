// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "DynSysGameSession.generated.h"

/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API ADynSysGameSession : public AGameSession
{
	GENERATED_BODY()
public:
	FString ApproveLogin(const FString& Options) override;
	
	
	
};
