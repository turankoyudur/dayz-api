class ApiBridgeConfig
{
    // Shared secret used by your external app (Node.js) to trust commands.
    // Stored in $profile:\ApiBridge\apibridge.cfg
    string ApiKey;

    // How often to write state.json
    int SnapshotIntervalSec;

    // How often to poll commands.json
    int CommandPollIntervalSec;

    // Safety limit for inventory enumeration
    int MaxItemsPerPlayer;

    // 0=quiet, 1=normal, 2=verbose
    int LogLevel;

    void SetDefaults()
    {
        ApiKey = ""; // generated on first run
        SnapshotIntervalSec = 2;
        CommandPollIntervalSec = 1;
        MaxItemsPerPlayer = 250;
        LogLevel = 1;
    }
};
