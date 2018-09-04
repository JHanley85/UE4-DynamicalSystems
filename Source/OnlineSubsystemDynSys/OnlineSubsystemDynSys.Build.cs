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
				"OnlineSubsystemUtils",
				"Json",
                "DynamicalSystems"
			}
			);
        PublicIncludePaths.AddRange(
       new string[] {
                "OnlineSubsystemDynSys/Public"
           // ... add public include paths required here ...
       }
       );


        PrivateIncludePaths.AddRange(
            new string[] {
                "OnlineSubsystemDynSys/Private",
				// ... add other private include paths required here ...
			}
            );
    }
}
