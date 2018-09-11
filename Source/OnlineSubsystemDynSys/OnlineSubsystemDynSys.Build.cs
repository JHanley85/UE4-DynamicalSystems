// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemDynSys : ModuleRules
{
	public OnlineSubsystemDynSys(ReadOnlyTargetRules Target) : base(Target)
    {
		PublicDefinitions.Add("ONLINESUBSYSTEMDYNSYS_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Sockets", 
				"OnlineSubsystem", 
				"Json",
                "Voice",
                "DynamicalSystems",
                "Networking",
            }
			);
        PublicIncludePaths.AddRange(
       new string[] {
                "OnlineSubsystemDynSys/Public",
                "OnlineSubsystemUtils/Public",
                "DynamicalSystems/Public",
                "PacketHandler/Public"
           // ... add public include paths required here ...
       }
       );
        PublicDependencyModuleNames.AddRange(
        new string[] {
                "DynamicalSystems",
                "OnlineSubsystemUtils",
                "Networking",
                "PacketHandler"

        }
        );

        

        PrivateIncludePaths.AddRange(
            new string[] {
                "OnlineSubsystemDynSys/Private",
                "OnlineSubsystemUtils/PRivate",
                "DynamicalSystems/Private"
				// ... add other private include paths required here ...
			}
            );
    }
}
