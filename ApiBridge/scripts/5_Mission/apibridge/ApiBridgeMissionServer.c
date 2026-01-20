modded class MissionServer
{
    ref ApiBridgeService m_ApiBridge;

    override void OnInit()
    {
        super.OnInit();
        m_ApiBridge = new ApiBridgeService;
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (m_ApiBridge)
            m_ApiBridge.OnUpdate(timeslice);
    }
};
