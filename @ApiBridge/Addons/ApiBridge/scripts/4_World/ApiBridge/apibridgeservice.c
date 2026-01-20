// apibridgeservice.c (4_World)

class ApiBridgeService
{
    private static ref ApiBridgeService s_Instance;

    private bool m_Inited;
    private int m_StateAccumMs;
    private int m_CmdAccumMs;

    static ApiBridgeService Get()
    {
        if (!s_Instance) s_Instance = new ApiBridgeService();
        return s_Instance;
    }

    void Init()
    {
        if (m_Inited) return;
        m_Inited = true;

        ApiBridgeConfig.LoadOrCreate();
		m_Config = new ApiBridgeConfig();
m_Config.LoadOrCreate();

Print("[ApiBridge] profilePath=" + GetGame().GetProfilePath());
Print("[ApiBridge] cfgPath=" + ApiBridgeConfig.ConfigPath());
Print("[ApiBridge] cfgExists=" + FileExist(ApiBridgeConfig.ConfigPath()).ToString());
        ApiBridgeConfig.EnsureDir();

        // write initial snapshots
        ApiBridgeStateCollector.WriteState();
    }

    void Tick(float timeslice)
    {
        if (!m_Inited) Init();

        ApiBridgeConfigData cfg = ApiBridgeConfig.Get();

        int dtMs = (int)(timeslice * 1000.0);
        if (dtMs < 0) dtMs = 0;

        m_StateAccumMs += dtMs;
        m_CmdAccumMs += dtMs;

        if (m_StateAccumMs >= cfg.SnapshotIntervalMs)
        {
            m_StateAccumMs = 0;
            ApiBridgeStateCollector.WriteState();
        }

        if (m_CmdAccumMs >= cfg.CommandPollIntervalMs)
        {
            m_CmdAccumMs = 0;
            ApiBridgeCommandProcessor.TickProcessCommands();
        }
    }

    void Shutdown()
    {
        m_Inited = false;
    }
}
