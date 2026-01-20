// apibridgemissionserver.c (5_Mission)

modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ApiBridgeService.Get().Init();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        ApiBridgeService.Get().Tick(timeslice);
    }

    override void OnMissionFinish()
    {
        ApiBridgeService.Get().Shutdown();
        super.OnMissionFinish();
    }
}
