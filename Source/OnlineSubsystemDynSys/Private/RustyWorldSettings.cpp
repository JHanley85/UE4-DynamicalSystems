// Fill out your copyright notice in the Description page of Project Settings.

#include "RustyWorldSettings.h"
#include "EngineUtils.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectIterator.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "RustyNetConnection.h"
#include "DynamicalSystemsSettings.h"



ARustyWorldSettings::ARustyWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	FString server = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().Server;
	FString  mumble = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().MumbleServer;

	TArray<FString> Out;
	mumble.ParseIntoArray(Out, TEXT(":"), true);
	Server = FURLAddress(server);
	Mumble = FURLAddress(mumble);
	
}
void ARustyWorldSettings::BeginPlay() {
	Super::BeginPlay();
}
void ARustyWorldSettings::CheckProperties() {
	if (!Connection) return;
	for (auto pair : PropertyMap) {
		if (PropChanged(pair.Key)) {
			TArray<uint8> Bytes;
			UProperty* Property = pair.Value;
			FBufferArchive buf = FBufferArchive();
			Property->Serialize(buf);
			SerializedProp.Add(pair.Key, buf);
			if (buf != FBufferArchive()) {
				((URustyNetConnection*)Connection)->Build_CallbackRep(Bytes, UserId, pair.Key, buf);
				
				((URustyNetConnection*)Connection)->SendServerRequest(ENetServerRequest::FunctionRep, Bytes, pair.Key);
			}
		}
	}
}