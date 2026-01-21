class ApiBridgeConfig
{
	string ApiKey = "CHANGE_ME";
	int PollIntervalMs = 250;          // how often commands.json is checked
	int StateWriteIntervalMs = 1000;    // how often state.json is written
	int LinkWriteIntervalMs = 1000;     // how often link.json is written

	bool EnableKick = true;
	bool EnableRestart = true;
	bool EnableShutdown = true;

	ref array<string> BanPlainIds = new array<string>; // soft-ban: will be disconnected on connect

	void EnsureDefaults()
	{
		if (!BanPlainIds) BanPlainIds = new array<string>;
	}
};
