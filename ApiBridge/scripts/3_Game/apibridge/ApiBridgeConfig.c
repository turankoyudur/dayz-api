class ApiBridgeConfig
{
    string ApiKey;
    int SnapshotIntervalSec;
    int LogLevel;

    void SetDefaults()
    {
        ApiKey = "CHANGE_ME";
        SnapshotIntervalSec = 5;
        LogLevel = 1;
    }
};
