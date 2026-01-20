// ApiBridgeModels.c (3_Game)

class ApiBridgeItemState
{
    string type;
    float health;
    float quantity;
    string parentType;
    string location;
};

class ApiBridgePlayerStats
{
    float health;
    float blood;
    float water;
    float energy;
};

class ApiBridgePlayerState
{
    string uid;
    string name;
    float x;
    float y;
    float z;
    ApiBridgePlayerStats stats;
    ref array<ref ApiBridgeItemState> inventory;
};

class ApiBridgeServerState
{
    string serverTime;
    int playerCount;
    float uptimeSeconds;
    ref array<ref ApiBridgePlayerState> players;
};

// ---- Commands ----

class ApiBridgeCommand
{
    string id;
    string type; // teleport, inv_add, inv_remove, inv_setqty, stats_set
    string uid;

    // teleport
    float x;
    float y;
    float z;

    // inventory
    string itemType;
    float quantity;
    int count;

    // stats
    float health;
    float blood;
    float water;
    float energy;
};

class ApiBridgeCommandBatch
{
    ref array<ref ApiBridgeCommand> commands;
};

class ApiBridgeCommandResult
{
    string id;
    bool success;
    string message;
};

class ApiBridgeResultBatch
{
    ref array<ref ApiBridgeCommandResult> results;
};
