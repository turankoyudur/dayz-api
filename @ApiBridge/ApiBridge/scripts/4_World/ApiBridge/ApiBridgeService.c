// ApiBridgeService.c (4_World)

class ApiBridgeService
{
    protected static ref ApiBridgeService s_Instance;

    protected ref ApiBridgeConfig m_Config;
    protected float m_UptimeSeconds;
    protected float m_SnapshotAcc;
    protected float m_CommandAcc;
    protected bool m_Inited;
    protected bool m_ExtensionEnabled;

    // Some DayZ builds expose CallExtension only on DayZGame (not on the CGame base type).
    // Use a small wrapper so the rest of the code stays simple.
    protected void CallApiBridgeExtension(string cmd)
    {
        if (!m_ExtensionEnabled)
            return;

        DayZGame dzg = DayZGame.Cast(GetGame());
        if (!dzg)
        {
            Print("[ApiBridge] DayZGame cast failed; cannot call extension");
            return;
        }

        string extOut = "";
        dzg.CallExtension(extOut, "ApiBridge", cmd);
        if (extOut != "")
            Print("[ApiBridge] extension out=" + extOut);
    }

    static ApiBridgeService Get()
    {
        if (!s_Instance)
            s_Instance = new ApiBridgeService();
        return s_Instance;
    }

    void Init()
    {
        if (m_Inited)
            return;

        MakeDirectory("$profile:ApiBridge");

        m_Config = new ApiBridgeConfig();
        m_Config.LoadOrCreate();

        // NOTE: Some DayZ script builds do not expose CGame.GetProfilePath().
        // We only rely on $profile: for file IO; logging absolute paths is optional.
        Print("[ApiBridge] cfgPath=" + ApiBridgeConfig.ConfigPath());
        Print("[ApiBridge] cfgExists=" + FileExist(ApiBridgeConfig.ConfigPath()).ToString());

        m_ExtensionEnabled = (m_Config && m_Config.EnableExtension == 1);
        if (m_ExtensionEnabled)
        {
            // Best effort init; ignore failures.
            // Extension resolves the profiles directory by parsing server command line (-profiles=...)
            string arg = "init|" + m_Config.BindIp + "|" + m_Config.Port.ToString() + "|" + m_Config.ApiKey;
            CallApiBridgeExtension(arg);
        }

        // ensure files exist
        if (!FileExist("$profile:ApiBridge\\commands.json"))
            JsonFileLoader<ApiBridgeCommandBatch>.JsonSaveFile("$profile:ApiBridge\\commands.json", new ApiBridgeCommandBatch());
        if (!FileExist("$profile:ApiBridge\\responses.json"))
            JsonFileLoader<ApiBridgeResultBatch>.JsonSaveFile("$profile:ApiBridge\\responses.json", new ApiBridgeResultBatch());

        m_Inited = true;
    }

    void Shutdown()
    {
        if (!m_Inited)
            return;

        CallApiBridgeExtension("shutdown");
        m_Inited = false;
    }

    void Tick(float timeslice)
    {
        if (!m_Inited)
            return;

        m_UptimeSeconds += timeslice;

        m_SnapshotAcc += timeslice;
        m_CommandAcc += timeslice;

		// Avoid ternary operator for broad DayZ Enforce Script compatibility
		float snapshotEvery = 1.0;
		float commandEvery = 0.25;
		if (m_Config)
		{
			if (m_Config.SnapshotIntervalMs > 0)
				snapshotEvery = m_Config.SnapshotIntervalMs / 1000.0;
			if (m_Config.CommandPollIntervalMs > 0)
				commandEvery = m_Config.CommandPollIntervalMs / 1000.0;
		}

        if (m_SnapshotAcc >= snapshotEvery)
        {
            m_SnapshotAcc = 0;
            ApiBridgeStateCollector.CollectAndSave(m_UptimeSeconds);
        }

        if (m_CommandAcc >= commandEvery)
        {
            m_CommandAcc = 0;
            ApiBridgeCommandProcessor.ProcessOnce();
        }
    }
};
