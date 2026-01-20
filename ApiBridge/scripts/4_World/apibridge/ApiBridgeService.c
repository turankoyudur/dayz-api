class ApiBridgeService
{
    ref ApiBridgeConfig m_Cfg;
    float m_Accum;

    void ApiBridgeService()
    {
        m_Cfg = new ApiBridgeConfig;
        ApiBridgeFile.LoadOrCreateConfig(m_Cfg);
        m_Accum = 0;
        Print("[ApiBridge] Initialized. Config: " + AB_CFG_PATH);
    }

    void OnUpdate(float timeslice)
    {
        m_Accum += timeslice;
        if (m_Accum < m_Cfg.SnapshotIntervalSec)
            return;

        m_Accum = 0;
        WriteSnapshot();
    }

    void WriteSnapshot()
    {
        ref ApiBridgeStateSnapshot state = new ApiBridgeStateSnapshot;
        state.world = GetGame().GetWorldName();
        state.serverTimeMs = GetGame().GetTime();

        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        state.playerCount = players.Count();

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players.Get(i));
            if (!pb) continue;

            PlayerIdentity id = pb.GetIdentity();

            ref ApiBridgePlayerSnapshot ps = new ApiBridgePlayerSnapshot;
            ps.pos = pb.GetPosition();
            ps.health = pb.GetHealth("", "");

            if (id)
            {
                ps.name = id.GetName();
                ps.id = id.GetId();
            }
            else
            {
                ps.name = "<noid>";
                ps.id = "";
            }

            state.players.Insert(ps);
        }

        ApiBridgeFile.SaveState(state);
    }
};
