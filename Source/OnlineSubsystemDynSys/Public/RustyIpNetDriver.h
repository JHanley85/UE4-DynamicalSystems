// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "IpNetDriver.h"
#include "RustyIpNetDriver.generated.h"

/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API URustyIpNetDriver : public UIpNetDriver
{
	GENERATED_BODY()

public:
	URustyIpNetDriver() {
		ConnectionTimeout = 500;
		InitialConnectTimeout = 500;
		RelevantTimeout = 500;
	}
	URustyIpNetDriver(const FObjectInitializer& ObjectInitializer);
	static URustyIpNetDriver Init() {
		return URustyIpNetDriver();
	}
	void InitializeSettings();
	FURL Server;
	FURL MumbleServer;
	FString AudioDevice;

	FSocket * CreateSocket() override;
	bool InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error) override;
	class URustyNetConnection* GetServerConnection();
};
