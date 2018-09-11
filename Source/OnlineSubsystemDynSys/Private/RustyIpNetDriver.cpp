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
	: Super(ObjectInitializer) {
	InitializeSettings();
}
void URustyIpNetDriver::InitializeSettings() {
	Server=FURL(nullptr, *UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().Server, ETravelType::TRAVEL_Absolute);
	MumbleServer=FURL(nullptr, *UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().MumbleServer, ETravelType::TRAVEL_Absolute);
	AudioDevice = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().AudioDevice;
}

bool URustyIpNetDriver::InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error)
{
	if (!InitBase(true, InNotify, ConnectURL, false, Error))
	{
		UE_LOG(LogNet, Warning, TEXT("Failed to init net driver ConnectURL: %s: %s"), *ConnectURL.ToString(), *Error);
		return false;
	}

	// Create new connection.
	ServerConnection = NewObject<UNetConnection>(GetTransientPackage(), NetConnectionClass);
	ServerConnection->InitLocalConnection(this, Socket, ConnectURL, USOCK_Pending);
	UE_LOG(LogNet, Log, TEXT("Game client on port %i, rate %i"), ConnectURL.Port, ServerConnection->CurrentNetSpeed);

	// Create channel zero.
	GetServerConnection()->CreateChannel(CHTYPE_Control, 1, 0);

	return true;
}
FSocket * URustyIpNetDriver::CreateSocket()
{
	// Create UDP socket and enable broadcasting.
	ISocketSubsystem* SocketSubsystem = GetSocketSubsystem();

	if (SocketSubsystem == NULL)
	{
		UE_LOG(LogNet, Warning, TEXT("UIpNetDriver::CreateSocket: Unable to find socket subsystem"));
		return NULL;
	}
	Socket= SocketSubsystem->CreateSocket(NAME_DGram, TEXT("Unreal"), true);
	return Socket;
}

URustyNetConnection* URustyIpNetDriver::GetServerConnection()
{
	return (URustyNetConnection*)ServerConnection;
}