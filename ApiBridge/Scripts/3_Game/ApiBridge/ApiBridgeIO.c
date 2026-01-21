class ApiBridgeIO
{
	static bool EnsureProfileDir()
	{
		// Directory creation is allowed only in $profile/$saves.
		// MakeDirectory returns bool but may return false if already exists.
		MakeDirectory(ApiBridgeConst.PROFILE_DIR);
		return true;
	}

	static bool LoadConfig(out ApiBridgeConfig cfg)
	{
		EnsureProfileDir();
		cfg = new ApiBridgeConfig;
		if (FileExist(ApiBridgeConst.CFG_PATH))
		{
			JsonFileLoader<ApiBridgeConfig>.JsonLoadFile(ApiBridgeConst.CFG_PATH, cfg);
		}
		cfg.EnsureDefaults();
		// If the file didn't exist, generate it.
		if (!FileExist(ApiBridgeConst.CFG_PATH))
		{
			JsonFileLoader<ApiBridgeConfig>.JsonSaveFile(ApiBridgeConst.CFG_PATH, cfg);
		}
		return true;
	}

	static void SaveConfig(ApiBridgeConfig cfg)
	{
		EnsureProfileDir();
		if (!cfg) return;
		cfg.EnsureDefaults();
		JsonFileLoader<ApiBridgeConfig>.JsonSaveFile(ApiBridgeConst.CFG_PATH, cfg);
	}

	static bool LoadCommands(out ApiBridgeCommandEnvelope env)
	{
		env = null;
		if (!FileExist(ApiBridgeConst.COMMANDS_PATH))
			return false;

		env = new ApiBridgeCommandEnvelope;
		JsonFileLoader<ApiBridgeCommandEnvelope>.JsonLoadFile(ApiBridgeConst.COMMANDS_PATH, env);
		return true;
	}

	static void AckCommands()
	{
		// Delete after processing; Node should only write when file is absent (acts as ack).
		if (FileExist(ApiBridgeConst.COMMANDS_PATH))
			DeleteFile(ApiBridgeConst.COMMANDS_PATH);
	}

	static void SaveResults(ApiBridgeResultsEnvelope res)
	{
		EnsureProfileDir();
		JsonFileLoader<ApiBridgeResultsEnvelope>.JsonSaveFile(ApiBridgeConst.RESULTS_PATH, res);
	}

	static void SaveState(ApiBridgeServerState state)
	{
		EnsureProfileDir();
		JsonFileLoader<ApiBridgeServerState>.JsonSaveFile(ApiBridgeConst.STATE_PATH, state);
	}

	static bool LoadNodeHeartbeat(out string nonce, out int t)
	{
		nonce = "";
		t = 0;
		if (!FileExist(ApiBridgeConst.NODE_HEARTBEAT_PATH))
			return false;

		// Minimal schema: { "t": <int ms>, "nonce": "..." }
		// Use a simple class to load.
		ref ApiBridgeNodeHeartbeat hb = new ApiBridgeNodeHeartbeat;
		JsonFileLoader<ApiBridgeNodeHeartbeat>.JsonLoadFile(ApiBridgeConst.NODE_HEARTBEAT_PATH, hb);
		if (!hb) return false;
		nonce = hb.nonce;
		t = hb.t;
		return true;
	}

	static void SaveLink(ApiBridgeLinkState link)
	{
		EnsureProfileDir();
		JsonFileLoader<ApiBridgeLinkState>.JsonSaveFile(ApiBridgeConst.LINK_PATH, link);
	}
};

class ApiBridgeNodeHeartbeat
{
	int t;
	string nonce;
};
