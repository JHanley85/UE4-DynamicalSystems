// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "RustyIpNetDriver.h"
#include "RustyNetConnection.h"
#include "NetClient.h"
#include "Runtime/Engine/Classes/GameFramework/GameSession.h"
#include "RustyLevelScriptActor.generated.h"

/**
 * 
 */

UCLASS(BLueprintType)
class ONLINESUBSYSTEMDYNSYS_API ARustyLevelScriptActor : public ALevelScriptActor

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

	UFUNCTION(BlueprintCallable)
		ARustyWorldSettings* GetSettings() {
		return settings;
	}
	virtual void BeginPlay() override;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, meta = (DisplayName = "Level is really ready"))
		void LevelIsReady();

	AGameModeBase* GameMode;
	AGameSession* GameSession;


	//UFUNCTION(BlueprintCallable, Category = OSS)
	//	virtual void OpenConnection() {
	//	UWorld* world = GetWorld();
	//	if (world != nullptr) {
	//		URustyIpNetDriver* driver = NewObject<URustyIpNetDriver>(this, "RustyIPNetDriver");
	//		world->SetNetDriver(driver);
	//		UE_LOG(LogTemp, Log, TEXT("3 Driver: {%s}"), *driver->GetClass()->GetName());
	//	}
	//}
	//void OnHandshake();
	//bool DoBunchOnce = false;
	//UFUNCTION(BlueprintCallable, Category = OSS)
	//	void SendMessage(TArray<uint8> bytes, int32 prepended);
	//UFUNCTION(BlueprintCallable, Category = OSS, meta = (AutoCreateRefTerm = "Value"))
	//	void SendServerRequest(ENetServerRequest RequestType, TArray<uint8> Value, int32 PropertyId = 0);
	//void RecvServerRequest(const NetBytes In);
	//void RecvMesage(const NetBytes In);
	virtual void ARustyLevelScriptActor::Tick(float DeltaTime) override;
	bool bIsSetup;
	UPROPERTY(BlueprintReadOnly,Category=World)
	ARustyWorldSettings* settings;
};