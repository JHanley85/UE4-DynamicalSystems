// Fill out your copyright notice in the Description page of Project Settings.

#include "RustyIpNetDriver.h"
#include "DynamicalSystemsSettings.h"
#include "IpNetDriver.h"
#include "Misc/CommandLine.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"
#include "PacketHandlers/StatelessConnectHandlerComponent.h"
#include "Engine/NetConnection.h"
#include "Engine/ChildConnection.h"
#include "SocketSubsystem.h"
#include "RustyNetConnection.h"
#include "HAL/LowLevelMemTracker.h"

#include "PacketAudit.h"

#include "IPAddress.h"
#include "Sockets.h"


URustyIpNetDriver::URustyIpNetDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, _ServerDesiredSocketReceiveBufferBytes(0x20000)
	, _ServerDesiredSocketSendBufferBytes(0x20000)
	, _ClientDesiredSocketReceiveBufferBytes(0x8000)
	, _ClientDesiredSocketSendBufferBytes(0x8000)
{
	bool bCanBindAll;
	LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

}


bool URustyIpNetDriver::InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error)
{
	if (!Super::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error))
	{
		return false;
	}

	ISocketSubsystem* SocketSubsystem = GetSocketSubsystem();
	if (SocketSubsystem == NULL)
	{
		UE_LOG(LogNet, Warning, TEXT("Unable to find socket subsystem"));
		return false;
	}

	// Derived types may have already allocated a socket

	// Create the socket that we will use to communicate with
	Socket = CreateSocket();

	if (Socket == NULL)
	{
		Socket = 0;
		Error = FString::Printf(TEXT("WinSock: socket failed (%i)"), (int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}
	if (SocketSubsystem->RequiresChatDataBeSeparate() == false &&
		Socket->SetBroadcast() == false)
	{
		Error = FString::Printf(TEXT("%s: setsockopt SO_BROADCAST failed (%i)"), SocketSubsystem->GetSocketAPIName(), (int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}

	if (Socket->SetReuseAddr(bReuseAddressAndPort) == false)
	{
		UE_LOG(LogNet, Log, TEXT("setsockopt with SO_REUSEADDR failed"));
	}

	if (Socket->SetRecvErr() == false)
	{
		UE_LOG(LogNet, Log, TEXT("setsockopt with IP_RECVERR failed"));
	}

	// Increase socket queue size, because we are polling rather than threading
	// and thus we rely on the OS socket to buffer a lot of data.
	int32 RecvSize = bInitAsClient ? _ClientDesiredSocketReceiveBufferBytes : _ServerDesiredSocketReceiveBufferBytes;
	int32 SendSize = bInitAsClient ? _ClientDesiredSocketSendBufferBytes : _ServerDesiredSocketSendBufferBytes;
	Socket->SetReceiveBufferSize(RecvSize, RecvSize);
	Socket->SetSendBufferSize(SendSize, SendSize);
	UE_LOG(LogInit, Log, TEXT("%s: Socket queue %i / %i"), SocketSubsystem->GetSocketAPIName(), RecvSize, SendSize);

	// Bind socket to our port.
	//LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
	LocalAddr = SocketSubsystem->GetLocalBindAddr(*GLog);

	LocalAddr->SetPort(bInitAsClient ? GetClientPort() : URL.Port);

	int32 AttemptPort = LocalAddr->GetPort();
	int32 BoundPort = SocketSubsystem->BindNextPort(Socket, *LocalAddr, MaxPortCountToTry + 1, 1);
	if (BoundPort == 0)
	{
		Error = FString::Printf(TEXT("%s: binding to port %i failed (%i)"), SocketSubsystem->GetSocketAPIName(), AttemptPort,
			(int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}
	if (Socket->SetNonBlocking() == false)
	{
		Error = FString::Printf(TEXT("%s: SetNonBlocking failed (%i)"), SocketSubsystem->GetSocketAPIName(),
			(int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}

	// Success.
	return true;
}
bool URustyIpNetDriver::InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error)
{
	//Super::InitConnect(InNotify, ConnectURL, Error);
	if (!InitBase(true, InNotify, ConnectURL, false, Error))
	{
		UE_LOG(LogNet, Warning, TEXT("Failed to init net driver ConnectURL: %s: %s"), *ConnectURL.ToString(), *Error);
		return false;
	}

	// Create new connection.
	
	
	ServerConnection = NewObject<UNetConnection>(GetTransientPackage(), NetConnectionClass);
	ServerConnection->InitLocalConnection(this, Socket, ConnectURL, USOCK_Pending);
	ServerConnection->Driver = this;

	UE_LOG(LogNet, Log, TEXT("Game client %s on %s port %i, rate %i"), *LocalAddr->ToString(true), *ConnectURL.Host,ConnectURL.Port, ServerConnection->CurrentNetSpeed);

	// Create channel zero.
	GetServerConnection()->CreateChannel(CHTYPE_Control, 1, 0);
	
	bNoTimeouts = true;

	return true;
}
FSocket * URustyIpNetDriver::CreateSocket()
{
	// Create UDP socket and enable broadcasting.
	ISocketSubsystem* SocketSubsystem = GetSocketSubsystem();

	if (SocketSubsystem == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("URustyIpNetDriver::CreateSocket: Unable to find socket subsystem"));
		return NULL;
	}
	Socket= SocketSubsystem->CreateSocket(NAME_DGram, TEXT("Unreal"), true);
	return Socket;
}

