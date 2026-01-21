class CfgPatches
{
	class ApiBridge
	{
		requiredAddons[] = {"DZ_Data"};
	};
};

class CfgMods
{
	class ApiBridge
	{
		type = "mod";
		// Script compilation / load order dependencies (NOT CfgPatches requiredAddons!)
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"ApiBridge/Scripts/3_Game"};
			};

			class missionScriptModule
			{
				value = "";
				files[] = {"ApiBridge/Scripts/5_Mission"};
			};
		};
	};
};
