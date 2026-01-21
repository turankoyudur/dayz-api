modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		if (GetGame() && GetGame().IsServer())
		{
			ApiBridgeServer.Get().InitIfNeeded();
			// Poll in CALL_CATEGORY_SYSTEM so it keeps running even when mission is paused.
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ApiBridgeServer.Get().Tick, 250, true);
			Print("[ApiBridge] Tick scheduled");
		}
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		ApiBridgeServer.Get().InitIfNeeded();
		if (identity && ApiBridgeServer.Get().IsBanned(identity))
		{
			Print("[ApiBridge] Banned player connected: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			GetGame().DisconnectPlayer(identity);
		}
	}
};
