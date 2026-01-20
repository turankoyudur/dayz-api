class CfgPatches
{
    class ApiBridge
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data","DZ_Scripts"};
    };
};

class CfgMods
{
    class ApiBridge
    {
        dir = "ApiBridge";
        name = "ApiBridge";
        type = "mod";
        dependencies[] = {"Game","World","Mission"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"ApiBridge/scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"ApiBridge/scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"ApiBridge/scripts/5_Mission"};
            };
        };
    };
};
