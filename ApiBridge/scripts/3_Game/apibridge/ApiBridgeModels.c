class ApiBridgeItemSnapshot
{
    string type;
    float quantity;
    float health;
    string parentType;
    string location;
};

class ApiBridgePlayerSnapshot
{
    string name;
    string id;
    vector pos;

    // Core vitals (always available)
    float health;
    float blood;
    float shock;

    // Optional vitals (some DayZ builds expose these via PlayerBase stats).
    // -1 means "not available".
    float water;
    float energy;

    int itemCount;
    ref array<ref ApiBridgeItemSnapshot> inventory;

    void ApiBridgePlayerSnapshot()
    {
        inventory = new array<ref ApiBridgeItemSnapshot>;
        water = -1;
        energy = -1;
    }
};

// Minimal metadata so external apps can verify that:
// 1) The mod is actively writing state.json
// 2) The mod is seeing Node heartbeats (two-way link)
class ApiBridgeBridgeInfo
{
    string modVersion;

    // When state.json was written (GetGame().GetTime() at write time)
    int lastStateWriteServerTimeMs;

    // When node_heartbeat.json was last read successfully
    int lastNodeSeenServerTimeMs;
    string lastNodeId;
    string lastNodeNonce;

    void ApiBridgeBridgeInfo()
    {
        modVersion = "filebridge-v2.1";
        lastStateWriteServerTimeMs = 0;
        lastNodeSeenServerTimeMs = 0;
        lastNodeId = "";
        lastNodeNonce = "";
    }
};

class ApiBridgeStateSnapshot
{
    string world;
    int serverTimeMs;
    int playerCount;
    ref array<ref ApiBridgePlayerSnapshot> players;

    // Bridge runtime metadata (useful for Node-side connectivity checks)
    ref ApiBridgeBridgeInfo bridge;

    void ApiBridgeStateSnapshot()
    {
        players = new array<ref ApiBridgePlayerSnapshot>;
        bridge = new ApiBridgeBridgeInfo();
    }
};

// Commands written by external app into $profile:\ApiBridge\commands.json
class ApiBridgeCommand
{
    // Unique client-generated id (string)
    string id;

    // Must match ApiBridgeConfig.ApiKey
    string apiKey;

    // teleport | inv_add | inv_remove | inv_setqty | set_health | set_blood | set_shock
    string type;

    // Player identity id (steamId)
    string playerId;

    // teleport
    float x;
    float y;
    float z;

    // inventory
    string itemType;
    float quantity;
    int count;

    // stat
    float value;
};

class ApiBridgeCommandBatch
{
    ref array<ref ApiBridgeCommand> commands;

    void ApiBridgeCommandBatch()
    {
        commands = new array<ref ApiBridgeCommand>;
    }
};

class ApiBridgeCommandResult
{
    string id;
    bool ok;
    string message;
};

class ApiBridgeCommandResults
{
    int serverTimeMs;
    ref array<ref ApiBridgeCommandResult> results;

    void ApiBridgeCommandResults()
    {
        results = new array<ref ApiBridgeCommandResult>;
    }
};
