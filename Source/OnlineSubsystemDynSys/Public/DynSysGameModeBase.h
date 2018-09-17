// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/CoreUObject/Public/UObject/CoreOnline.h"
#include "DynSysGameModeBase.generated.h"


/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API ADynSysGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADynSysGameModeBase(const FObjectInitializer& ObjectInitializer);
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	APlayerController* SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation);
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category=Network)
		TSubclassOf<APlayerController> RemotePlayerControllerClass;

};
