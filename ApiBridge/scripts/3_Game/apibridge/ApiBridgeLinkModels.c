// Models used for connectivity monitoring between the DayZ mod and an external app (e.g. Node.js)

// Written by Node.js into $profile:\ApiBridge\node_heartbeat.json
class ApiBridgeNodeHeartbeat
{
    // Must match ApiBridgeConfig.ApiKey
    string apiKey;

    // Caller-defined stable identifier (e.g. hostname or service name)
    string nodeId;

    // Random nonce that the mod should echo back
    string nonce;

    // Optional: client timestamp (use string because unix ms doesn't fit in Enforce 'int')
    string sentAt;

    void ApiBridgeNodeHeartbeat()
    {
        apiKey = "";
        nodeId = "";
        nonce = "";
        sentAt = "";
    }
};

// Written by the mod into $profile:\ApiBridge\bridge_heartbeat.json
class ApiBridgeBridgeHeartbeat
{
    string modVersion;

    // GetGame().GetTime() (ms since server start)
    int serverTimeMs;

    // Node heartbeat info as last seen by the mod
    int lastNodeSeenServerTimeMs;
    string nodeId;
    string nonceEcho;

    void ApiBridgeBridgeHeartbeat()
    {
        modVersion = "filebridge-v2.1";
        serverTimeMs = 0;
        lastNodeSeenServerTimeMs = 0;
        nodeId = "";
        nonceEcho = "";
    }
};
