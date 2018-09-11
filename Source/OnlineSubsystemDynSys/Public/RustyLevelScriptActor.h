// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "RustyIpNetDriver.h"
#include "RustyNetConnection.h"
#include "NetClient.h"
#include "RustyLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS(BLueprintType)
class ONLINESUBSYSTEMDYNSYS_API ARustyLevelScriptActor : public ALevelScriptActor, public FNetworkNotify

{
	GENERATED_BODY()
public:
	ARustyLevelScriptActor() {
		UWorld* world = GetWorld();
		if (world != nullptr) {
			if (world->NetDriver != nullptr) {
				UE_LOG(LogTemp, Log, TEXT("2 Driver: {%s}"), *world->NetDriver->GetClass()->GetName());
			}
		}
	}
	ARustyLevelScriptActor(const FObjectInitializer& ObjectInitializer);
	UFUNCTION(BlueprintCallable)
		void RegisterActors();

	virtual void BeginPlay() override;
	UFUNCTION(BlueprintCallable, Category = OSS)
		virtual void OpenConnection() {
		UWorld* world = GetWorld();
		if (world != nullptr) {
			URustyIpNetDriver* driver = NewObject<URustyIpNetDriver>(this, "RustyIPNetDriver");
			world->SetNetDriver(driver);
			UE_LOG(LogTemp, Log, TEXT("3 Driver: {%s}"), *driver->GetClass()->GetName());
		}
	}
	//~ Begin FNetworkNotify Interface
	virtual EAcceptConnection::Type NotifyAcceptingConnection() override;
	virtual void NotifyAcceptedConnection(class UNetConnection* Connection) override;
	virtual bool NotifyAcceptingChannel(class UChannel* Channel) override;
	virtual void NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch) override;
	//~ End FNetworkNotify Interface
	URustyIpNetDriver* NetDriver;
	FSocket* OutSocket;
	FSocket* ListenSocket;
	UChannel* controlchannel;
	void OnHandshake();
	bool DoBunchOnce = false;
	UFUNCTION(BlueprintCallable, Category = OSS)
		void SendMessage(TArray<uint8> bytes, int32 prepended);
	UFUNCTION(BlueprintCallable, Category = OSS, meta = (AutoCreateRefTerm = "Value"))
		void SendServerRequest(ENetServerRequest RequestType, TArray<uint8> Value, int32 PropertyId = 0);
	void RecvServerRequest(const NetBytes In);
	void RecvMesage(const NetBytes In);
	virtual void ARustyLevelScriptActor::Tick(float DeltaTime) override;
	ARustyWorldSettings* settings;
	URustyNetConnection* connection;
};
