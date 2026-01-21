class ApiBridgePlayerState
{
	string id;      // hashed id (GetId)
	string plainId; // plain id (GetPlainId)
	string name;
	vector pos;
	float health;
};

class ApiBridgeServerState
{
	int apiVersion;
	int serverTimeMs; // GetGame().GetTime()
	string worldName;
	int playerCount;
	ref array<ref ApiBridgePlayerState> players;

	void ApiBridgeServerState()
	{
		apiVersion = ApiBridgeConst.API_VERSION;
		players = new array<ref ApiBridgePlayerState>;
	}
};

class ApiBridgeLinkState
{
	int apiVersion;
	int serverTimeMs;
	int lastNodeHeartbeatMs;
	string lastNodeNonce;
	string status;

	void ApiBridgeLinkState()
	{
		apiVersion = ApiBridgeConst.API_VERSION;
		status = "ok";
	}
};
