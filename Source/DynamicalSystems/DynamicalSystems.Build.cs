// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System.IO;
using UnrealBuildTool;

public class DynamicalSystems : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public DynamicalSystems(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "DynamicalSystems/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "DynamicalSystems/Private",
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "PhysX",
                "APEX",
                "RustyDynamics",
                "Projects",
                "InputCore",
                "AnimGraphRuntime",
                "AnimationCore",
                "GameplayAbilities",
                "GameplayTags",
                "Sockets",
                "OnlineSubsystem"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

				// ... add private dependencies that you statically link with here ...
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
        LoadDynSysLibs(Target);
        //Lib
        string PlatformString = Target.Platform.ToString();
        PublicAdditionalLibraries.Add(Path.Combine(RustyDynamicsPath, "RustyDynamics.dll.lib"));

    }
    private string RustyDynamicsPath
    {
        get
        {
            return Path.GetFullPath(Path.Combine(ThirdPartyPath, "RustyDynamics", "target", "Release"));
        }
    }
    public void LoadDynSysLibs(ReadOnlyTargetRules Target)
    {
        //RustyDynamics
        string PlatformString = Target.Platform.ToString();
        AddDependency(Target, Path.Combine(BinaryFolderPath, PlatformString), "ssleay32.dll", true);
        AddDependency(Target, Path.Combine(BinaryFolderPath, PlatformString), "libeay32.dll", true);
        AddDependency(Target, Path.Combine(BinaryFolderPath, PlatformString), "ovraudio64.dll", true);
        AddDependency(Target, RustyDynamicsPath, "RustyDynamics.dll", false);
    }
    void AddDependency(ReadOnlyTargetRules Target, string DllPath, string DLLName, bool DoCopy)
    {
        string PlatformString = Target.Platform.ToString();
        string PluginDLLPath = Path.Combine(DllPath, DLLName);
        if (DoCopy)
        {
            CopyToProjectBinaries(PluginDLLPath, Target);
            PluginDLLPath = Path.GetFullPath(Path.Combine(GetUProjectPath, "Binaries", PlatformString, DLLName));

        }
        System.Console.WriteLine("Project plugin: " + DLLName + " detected, using dll at " + PluginDLLPath);


        RuntimeDependencies.Add(PluginDLLPath);
    }
    private string BinaryFolderPath
    {
        get
        {
            return Path.GetFullPath(Path.Combine(ModulePath, "../../Binaries/"));
        }
    }
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../Source/ThirdParty/")); }
    }

    public string GetUProjectPath
    {
        get
        {
            return System.IO.Path.Combine(ModuleDirectory, "../../../../");
            return Directory.GetParent(ModulePath).Parent.Parent.ToString();
        }
    }
    private void CopyToProjectBinaries(string Filepath, ReadOnlyTargetRules Target)
    {
        System.Console.WriteLine("uprojectpath is: " + Path.GetFullPath(GetUProjectPath));
        System.Console.WriteLine("ThirdPartyPath is: " + Path.GetFullPath(ThirdPartyPath));
        string binariesDir = Path.Combine(GetUProjectPath, "Binaries", Target.Platform.ToString());
        string filename = Path.GetFileName(Filepath);
        string fullBinariesDir = Path.GetFullPath(binariesDir);

        if (!Directory.Exists(fullBinariesDir))
            Directory.CreateDirectory(fullBinariesDir);

        if (!File.Exists(Path.Combine(fullBinariesDir, filename)))
        {
            System.Console.WriteLine("DynSys: Copied from " + Filepath + ", to " + Path.Combine(fullBinariesDir, filename));
            File.Copy(Filepath, Path.Combine(fullBinariesDir, filename), true);
        }
    }
}
