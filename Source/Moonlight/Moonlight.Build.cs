using UnrealBuildTool;
using System.Collections.Generic;
using System.IO;

public class Moonlight : ModuleRules {

	private string moonvdecPath {
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/moonvdec/")); }
	}

	public Moonlight(ReadOnlyTargetRules Target) : base(Target) {
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "RHI", "RenderCore"});
		PrivatePCHHeaderFile = "Private/MoonlightPCH.h";

		if (Target.Platform != UnrealTargetPlatform.Win64) {
			throw new System.Exception("This plugin is only available for Win64 right now.");
		}

		PrivateIncludePaths.Add(Path.Combine(moonvdecPath, "include/"));
		
		PublicLibraryPaths.Add(Path.Combine(moonvdecPath, "lib/Win64/"));
		PublicAdditionalLibraries.Add("moonvdec.lib");

		//PublicDelayLoadDLLs.Add("libeay32.dll");
		//PublicDelayLoadDLLs.Add("ssleay32.dll");
		PublicDelayLoadDLLs.Add("moonvdec.dll");

		addDependency("Win64", "moonvdec.dll");
		addDependency("Win64", "libcrypto-1_1-x64.dll");
		//addDependency("Win64", "libeay32.dll");
		//addDependency("Win64", "ssleay32.dll");

		if (File.Exists(Path.Combine(moonvdecPath, "lib/Win64/Qt5Core.dll"))) {
			addDependency("Win64", "Qt5Core.dll");
		} else {
			addDependency("Win64", "Qt5Cored.dll");
		}

		if (File.Exists(Path.Combine(moonvdecPath, "lib/Win64/Qt5Network.dll"))) {
			addDependency("Win64", "Qt5Network.dll");
		} else {
			addDependency("Win64", "Qt5Networkd.dll");
		}
	}
	
	private void addDependency(string arch, string lib) {
		RuntimeDependencies.Add(Path.Combine(moonvdecPath, "lib/", arch, lib));
	}
}
