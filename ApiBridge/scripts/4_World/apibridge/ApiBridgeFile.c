class ApiBridgeFile
{
    static void EnsureProfileDir()
    {
        // Safe to call repeatedly.
        MakeDirectory(AB_PROFILE_DIR);
    }

    static string GenerateApiKey(int len)
    {
        string chars = "0123456789abcdef";
        string key = "";

        for (int i = 0; i < len; i++)
        {
            int idx = Math.RandomInt(0, chars.Length());
            key = key + chars.Substring(idx, 1);
        }

        return key;
    }

    static void LoadOrCreateConfig(out ApiBridgeConfig cfg)
    {
        EnsureProfileDir();
        cfg.SetDefaults();

        if (!FileExist(AB_CFG_PATH))
        {
            if (cfg.ApiKey == "")
                cfg.ApiKey = GenerateApiKey(32);

            JsonFileLoader<ApiBridgeConfig>.JsonSaveFile(AB_CFG_PATH, cfg);
            return;
        }

        JsonFileLoader<ApiBridgeConfig>.JsonLoadFile(AB_CFG_PATH, cfg);

        // In case user deleted key field
        if (cfg.ApiKey == "")
        {
            cfg.ApiKey = GenerateApiKey(32);
            JsonFileLoader<ApiBridgeConfig>.JsonSaveFile(AB_CFG_PATH, cfg);
        }
    }

    static void SaveState(ApiBridgeStateSnapshot state)
    {
        EnsureProfileDir();
        JsonFileLoader<ApiBridgeStateSnapshot>.JsonSaveFile(AB_STATE_PATH, state);
    }

    static bool TryLoadCommands(out ApiBridgeCommandBatch batch)
    {
        batch = new ApiBridgeCommandBatch();

        if (!FileExist(AB_COMMANDS_PATH))
            return false;

        JsonFileLoader<ApiBridgeCommandBatch>.JsonLoadFile(AB_COMMANDS_PATH, batch);

        // Remove commands file so we don't process twice.
        DeleteFile(AB_COMMANDS_PATH);

        if (!batch || !batch.commands || batch.commands.Count() == 0)
            return false;

        return true;
    }

    static void SaveResults(ApiBridgeCommandResults results)
    {
        EnsureProfileDir();
        JsonFileLoader<ApiBridgeCommandResults>.JsonSaveFile(AB_RESULTS_PATH, results);
    }

    // --- Link check / heartbeat ---

    static bool TryLoadNodeHeartbeat(out ApiBridgeNodeHeartbeat hb)
    {
        hb = new ApiBridgeNodeHeartbeat();
        if (!FileExist(AB_NODE_HB_PATH))
            return false;

        // JsonLoadFile returns void on some DayZ builds; avoid capturing return.
        JsonFileLoader<ApiBridgeNodeHeartbeat>.JsonLoadFile(AB_NODE_HB_PATH, hb);

        if (!hb)
            return false;
        if (hb.apiKey == "")
            return false;

        return true;
    }

    static void SaveBridgeHeartbeat(ApiBridgeBridgeHeartbeat hb)
    {
        EnsureProfileDir();
        JsonFileLoader<ApiBridgeBridgeHeartbeat>.JsonSaveFile(AB_BRIDGE_HB_PATH, hb);
    }
};
