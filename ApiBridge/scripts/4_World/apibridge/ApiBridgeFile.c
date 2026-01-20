class ApiBridgeFile
{
    static void EnsureProfileDir()
    {
        // Safe to call repeatedly.
        MakeDirectory(AB_PROFILE_DIR);
    }

    static void LoadOrCreateConfig(out ApiBridgeConfig cfg)
    {
        EnsureProfileDir();
        cfg.SetDefaults();

        if (!FileExist(AB_CFG_PATH))
        {
            JsonFileLoader<ApiBridgeConfig>.JsonSaveFile(AB_CFG_PATH, cfg);
            return;
        }

        JsonFileLoader<ApiBridgeConfig>.JsonLoadFile(AB_CFG_PATH, cfg);
    }

    static void SaveState(ApiBridgeStateSnapshot state)
    {
        EnsureProfileDir();
        JsonFileLoader<ApiBridgeStateSnapshot>.JsonSaveFile(AB_STATE_PATH, state);
    }
};
