// ApiBridgeConfig.c (3_Game)

class ApiBridgeConfig
{
    // Defaults
    string ApiKey;
    string BindIp;
    int Port;
    int SnapshotIntervalMs;
    int CommandPollIntervalMs;
    int EnableExtension;

    static string DirPath()
    {
        return "$profile:ApiBridge";
    }

    static string ConfigPath()
    {
        return "$profile:ApiBridge\\apibridge.cfg";
    }

    void SetDefaults()
    {
        ApiKey = GenerateApiKey(32);
        BindIp = "127.0.0.1";
        Port = 8192;
        SnapshotIntervalMs = 1000;
        CommandPollIntervalMs = 250;
        EnableExtension = 1;
    }

    void LoadOrCreate()
    {
        MakeDirectory(DirPath());

        string path = ConfigPath();
        if (!FileExist(path))
        {
            SetDefaults();
            Save();
            return;
        }

        // load key=value lines
        FileHandle f = OpenFile(path, FileMode.READ);
        if (f == 0)
        {
            // can't open -> keep defaults
            SetDefaults();
            Save();
            return;
        }

        SetDefaults(); // start from defaults, then override

        string line;
        while (FGets(f, line) > 0)
        {
            line.Trim();
			// Avoid string indexing / char literals for broad DayZ version compatibility
			if (line == "")
				continue;
			if (line.Length() > 0 && line.Substring(0, 1) == "#")
				continue;

            int eq = line.IndexOf("=");
            if (eq <= 0)
                continue;

            string k = line.Substring(0, eq);
            string v = line.Substring(eq + 1, line.Length() - eq - 1);
            k.Trim();
            v.Trim();

            if (k == "ApiKey") ApiKey = v;
            else if (k == "BindIp") BindIp = v;
            else if (k == "Port") Port = v.ToInt();
            else if (k == "SnapshotIntervalMs") SnapshotIntervalMs = v.ToInt();
            else if (k == "CommandPollIntervalMs") CommandPollIntervalMs = v.ToInt();
            else if (k == "EnableExtension") EnableExtension = v.ToInt();
        }

        CloseFile(f);

        // ensure key exists
        if (ApiKey == "")
        {
            ApiKey = GenerateApiKey(32);
            Save();
        }
    }

    void Save()
    {
        MakeDirectory(DirPath());
        FileHandle f = OpenFile(ConfigPath(), FileMode.WRITE);
        if (f == 0) return;

        FPrintln(f, "# ApiBridge config");
        FPrintln(f, "ApiKey=" + ApiKey);
        FPrintln(f, "BindIp=" + BindIp);
        FPrintln(f, "Port=" + Port.ToString());
        FPrintln(f, "SnapshotIntervalMs=" + SnapshotIntervalMs.ToString());
        FPrintln(f, "CommandPollIntervalMs=" + CommandPollIntervalMs.ToString());
        FPrintln(f, "EnableExtension=" + EnableExtension.ToString());

        CloseFile(f);
    }

    static string GenerateApiKey(int len)
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
};
