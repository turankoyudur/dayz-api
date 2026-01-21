class ApiBridgeCommand
{
	string id;
	string type;          // e.g. "ping", "server.status", "server.restart", "player.kick", "server.message"
	string playerPlainId; // target player (steam64 / plain id)
	string message;       // for server.message
	vector pos;           // for teleport etc (future)
};

class ApiBridgeCommandEnvelope
{
	int apiVersion;
	string apiKey;
	ref array<ref ApiBridgeCommand> commands;

	void ApiBridgeCommandEnvelope()
	{
		apiVersion = ApiBridgeConst.API_VERSION;
		commands = new array<ref ApiBridgeCommand>;
	}
};

class ApiBridgeCommandResult
{
	string id;
	bool ok;
	string error;
	string data;
	ref ApiBridgeServerState serverState;
};

class ApiBridgeResultsEnvelope
{
	int apiVersion;
	int serverTimeMs;
	ref array<ref ApiBridgeCommandResult> results;

	void ApiBridgeResultsEnvelope()
	{
		apiVersion = ApiBridgeConst.API_VERSION;
		results = new array<ref ApiBridgeCommandResult>;
	}
};
