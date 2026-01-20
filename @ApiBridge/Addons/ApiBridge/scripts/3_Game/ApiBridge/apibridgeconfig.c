// apibridgeconfig.c (3_Game)

class ApiBridgeConfigData
{
    string ApiKey;
    int SnapshotIntervalMs;
    int CommandPollIntervalMs;
    string BindIp;
    int Port;
}

class ApiBridgeConfig
{
    private static ref ApiBridgeConfigData s_Data;

    static string Dir() { return "$profile:ApiBridge"; }
    static string ConfigJsonPath() { return "$profile:ApiBridge/config.json"; }
    static string CfgPath() { return "$profile:ApiBridge/apibridge.cfg"; }

    static void EnsureDir()
    {
        if (!FileExist(Dir()))
        {
            MakeDirectory(Dir());
        }
    }

    static ApiBridgeConfigData Get()
    {
        if (!s_Data)
        {
            LoadOrCreate();
        }
        return s_Data;
    }

    static void LoadOrCreate()
    {
        EnsureDir();
        s_Data = new ApiBridgeConfigData();

        // defaults
        s_Data.ApiKey = GenerateApiKey(32);
        s_Data.SnapshotIntervalMs = 1000;
        s_Data.CommandPollIntervalMs = 250;
        s_Data.BindIp = "127.0.0.1";
        s_Data.Port = 8192;

        // Prefer JSON if present (easiest / most robust)
        if (FileExist(ConfigJsonPath()))
        {
            JsonFileLoader<ApiBridgeConfigData>.JsonLoadFile(ConfigJsonPath(), s_Data);
            WriteCfgFromData();
            return;
        }

        // If cfg exists but JSON doesn't, we still generate JSON from defaults.
        // (You can expand this to parse cfg if you want.)
        JsonFileLoader<ApiBridgeConfigData>.JsonSaveFile(ConfigJsonPath(), s_Data);
        WriteCfgFromData();
    }

    private static void WriteCfgFromData()
    {
        FileHandle fh = OpenFile(CfgPath(), FileMode.WRITE);
        if (!fh) return;

        FPrintln(fh, "ApiKey=" + s_Data.ApiKey);
        FPrintln(fh, "SnapshotIntervalMs=" + s_Data.SnapshotIntervalMs.ToString());
        FPrintln(fh, "CommandPollIntervalMs=" + s_Data.CommandPollIntervalMs.ToString());
        FPrintln(fh, "BindIp=" + s_Data.BindIp);
        FPrintln(fh, "Port=" + s_Data.Port.ToString());

        CloseFile(fh);
    }

    private static string GenerateApiKey(int len)
    {
        string chars = "0123456789abcdef";
        string result = "";

        for (int i = 0; i < len; i++)
        {
            int idx = Math.RandomInt(0, chars.Length());
            result += chars.Substring(idx, 1);
        }

        return result;
    }
}
