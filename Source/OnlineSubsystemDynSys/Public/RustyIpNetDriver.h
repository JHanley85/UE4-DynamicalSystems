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

	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error)override;

	FSocket * CreateSocket() override;
	virtual bool InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error) override;

private:
	/** Number of bytes that will be passed to FSocket::SetReceiveBufferSize when initializing a server. */
	UPROPERTY(Config)
		uint32 _ServerDesiredSocketReceiveBufferBytes;

	/** Number of bytes that will be passed to FSocket::SetSendBufferSize when initializing a server. */
	UPROPERTY(Config)
		uint32 _ServerDesiredSocketSendBufferBytes;

	/** Number of bytes that will be passed to FSocket::SetReceiveBufferSize when initializing a client. */
	UPROPERTY(Config)
		uint32 _ClientDesiredSocketReceiveBufferBytes;

	/** Number of bytes that will be passed to FSocket::SetSendBufferSize when initializing a client. */
	UPROPERTY(Config)
		uint32 _ClientDesiredSocketSendBufferBytes;

};
