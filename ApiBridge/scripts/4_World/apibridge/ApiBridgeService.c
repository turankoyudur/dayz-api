class ApiBridgeService
{
    ref ApiBridgeConfig m_Cfg;
    ref ApiBridgeStateCollector m_Collector;
    ref ApiBridgeCommandProcessor m_CommandProc;

    float m_SnapshotAccum;
    float m_CommandAccum;

    // Node <-> mod connectivity (updated when node_heartbeat.json is read)
    string m_LastNodeId;
    string m_LastNodeNonce;
    int m_LastNodeSeenServerTimeMs;

    void ApiBridgeService()
    {
        m_Cfg = new ApiBridgeConfig;
        ApiBridgeFile.LoadOrCreateConfig(m_Cfg);

        m_Collector = new ApiBridgeStateCollector(m_Cfg);
        m_CommandProc = new ApiBridgeCommandProcessor(m_Cfg);

        m_SnapshotAccum = 0;
        m_CommandAccum = 0;

        m_LastNodeId = "";
        m_LastNodeNonce = "";
        m_LastNodeSeenServerTimeMs = 0;

        Print("[ApiBridge] Initialized. Config: " + AB_CFG_PATH);
    }

    void OnUpdate(float timeslice)
    {
        m_SnapshotAccum = m_SnapshotAccum + timeslice;
        m_CommandAccum = m_CommandAccum + timeslice;

        if (m_SnapshotAccum >= m_Cfg.SnapshotIntervalSec)
        {
            m_SnapshotAccum = 0;
            WriteSnapshot();
        }

        if (m_CommandAccum >= m_Cfg.CommandPollIntervalSec)
        {
            m_CommandAccum = 0;
            CheckNodeHeartbeat();
            if (m_CommandProc)
                m_CommandProc.Tick();
        }
    }

    void WriteSnapshot()
    {
        if (!m_Collector)
            return;

        ApiBridgeStateSnapshot state = m_Collector.Collect();

        // Attach link-check metadata
        if (state && state.bridge)
        {
            state.bridge.modVersion = "filebridge-v2.1";
            state.bridge.lastStateWriteServerTimeMs = GetGame().GetTime();
            state.bridge.lastNodeSeenServerTimeMs = m_LastNodeSeenServerTimeMs;
            state.bridge.lastNodeId = m_LastNodeId;
            state.bridge.lastNodeNonce = m_LastNodeNonce;
        }

        ApiBridgeFile.SaveState(state);

        // Also write a small heartbeat file that Node can monitor
        WriteBridgeHeartbeat();

        if (m_Cfg.LogLevel >= 2)
            Print("[ApiBridge] state.json updated. players=" + state.playerCount.ToString());
    }

    void CheckNodeHeartbeat()
    {
        ApiBridgeNodeHeartbeat hb;
        if (!ApiBridgeFile.TryLoadNodeHeartbeat(hb))
            return;

        if (!hb)
            return;
        if (hb.apiKey != m_Cfg.ApiKey)
            return;

        m_LastNodeId = hb.nodeId;
        m_LastNodeNonce = hb.nonce;
        m_LastNodeSeenServerTimeMs = GetGame().GetTime();

        if (m_Cfg.LogLevel >= 3)
            Print("[ApiBridge] node_heartbeat seen. nodeId=" + m_LastNodeId);
    }

    void WriteBridgeHeartbeat()
    {
        ApiBridgeBridgeHeartbeat bh = new ApiBridgeBridgeHeartbeat();
        bh.modVersion = "filebridge-v2.1";
        bh.serverTimeMs = GetGame().GetTime();
        bh.lastNodeSeenServerTimeMs = m_LastNodeSeenServerTimeMs;
        bh.nodeId = m_LastNodeId;
        bh.nonceEcho = m_LastNodeNonce;
        ApiBridgeFile.SaveBridgeHeartbeat(bh);
    }
};
