// apibridgemissionserver.c (5_Mission)
Print("[ApiBridge] MissionServer OnInit reached");
modded class MissionServer
{
	Print("[ApiBridge] MissionServer OnInit reached");
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
