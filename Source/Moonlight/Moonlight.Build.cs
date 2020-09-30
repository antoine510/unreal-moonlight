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

		if (Target.Platform != UnrealTargetPlatform.Win64) {
			throw new System.Exception("This plugin is only available for Win64 right now.");
		}

		PrivateIncludePaths.Add(Path.Combine(moonvdecPath, "include/"));
		
		addLibrary("Win64", "moonvdec.lib");

		PublicDelayLoadDLLs.Add("moonvdec.dll");

		addDependency("Win64", "moonvdec.dll");
		addDependency("Win64", "libcrypto-1_1-x64.dll");

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
	
	private void addLibrary(string arch, string lib) {
		PublicAdditionalLibraries.Add(Path.Combine(moonvdecPath, "lib/", arch, lib));
	}
	
	private void addDependency(string arch, string lib) {
		RuntimeDependencies.Add(Path.Combine(moonvdecPath, "lib/", arch, lib));
	}
}
