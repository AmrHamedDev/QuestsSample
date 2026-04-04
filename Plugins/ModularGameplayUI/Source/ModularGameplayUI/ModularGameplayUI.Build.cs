// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ModularGameplayUI : ModuleRules
{
	public ModularGameplayUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"CommonLoadingScreen",
				"Core",
				"ApplicationCore",
				"EnhancedInput",
				"GameplayTags",
				"ModularGameplayData",
				"ModularGameplayExperiences",
				"UIExtension",
				"DeveloperSettings"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CommonGame",
				"CommonInput",
				"CommonUI",
				"CommonUser",
				"ControlFlows",
				"CoreUObject",
				"Engine",
				"GameFeatures",
				"GameplayAbilities",
				"ModularGameplay",
				"ModularGameplayData",
				"ModularGameplayExperiences",
				"Slate",
				"SlateCore",
				"UIExtension",
				"UMG",
				"WebRTC",
				"InputCore",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
