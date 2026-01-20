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
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ApiBridge";
        credits = "";
        author = "";
        authorID = "0";
        version = "1.0";
        extra = 0;
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
