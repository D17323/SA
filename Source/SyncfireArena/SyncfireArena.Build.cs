// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SyncfireArena : ModuleRules
{
	public SyncfireArena(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"Niagara", 
			"AIModule", 
			"UMG", 
			"Slate", 
			"SlateCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemSteam",
			// UE 5.8 的 SteamNetDriver / SteamNetConnection 在独立的 SocketSubsystemSteamIP 模块里。
			// 它只在运行时通过 ini 里的 NetDriverDefinitions 按类路径加载，代码没有直接引用，
			// 因此必须显式声明为依赖，否则游戏（单体）构建不会把它编进 exe，运行时就会回退到 IpNetDriver。
			"SocketSubsystemSteamIP"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
