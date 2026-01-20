// ApiBridgeService.c (4_World)

class ApiBridgeService
{
    protected static ref ApiBridgeService s_Instance;

    protected ref ApiBridgeConfig m_Config;
    protected float m_UptimeSeconds;
    protected float m_SnapshotAcc;
    protected float m_CommandAcc;
    protected bool m_Inited;

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

        Print("[ApiBridge] profilePath=" + GetGame().GetProfilePath());
        Print("[ApiBridge] cfgPath=" + ApiBridgeConfig.ConfigPath());
        Print("[ApiBridge] cfgExists=" + FileExist(ApiBridgeConfig.ConfigPath()).ToString());

        if (m_Config.EnableExtension == 1)
        {
            string extOut = "";
            // Best effort init; ignore failures
            string arg = "init|" + GetGame().GetProfilePath() + "|" + m_Config.BindIp + "|" + m_Config.Port.ToString() + "|" + m_Config.ApiKey;
            GetGame().CallExtension(extOut, "ApiBridge", arg);
            Print("[ApiBridge] extension init out=" + extOut);
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

        string extOut = "";
        GetGame().CallExtension(extOut, "ApiBridge", "shutdown");
        m_Inited = false;
    }

    void Tick(float timeslice)
    {
        if (!m_Inited)
            return;

        m_UptimeSeconds += timeslice;

        m_SnapshotAcc += timeslice;
        m_CommandAcc += timeslice;

        float snapshotEvery = (m_Config && m_Config.SnapshotIntervalMs > 0) ? (m_Config.SnapshotIntervalMs / 1000.0) : 1.0;
        float commandEvery  = (m_Config && m_Config.CommandPollIntervalMs > 0) ? (m_Config.CommandPollIntervalMs / 1000.0) : 0.25;

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
