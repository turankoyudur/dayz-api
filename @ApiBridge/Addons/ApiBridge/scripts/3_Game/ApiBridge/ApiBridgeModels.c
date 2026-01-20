// ApiBridgeModels.c

class ApiBridgeItemState
{
    string type;
    float health;
    int quantity;
    string location;  // hands / cargo / attachment / ground / unknown
    string parentType;
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
    ref array<float> pos; // [x,y,z]
    ref ApiBridgePlayerStats stats;
    ref array<ref ApiBridgeItemState> inventory;
};

class ApiBridgeServerInfo
{
    int playerCount;
    int maxPlayers; // 0 if unknown
    int uptimeMs;
};

class ApiBridgeState
{
    int version;
    int ts; // server time ms is OK
    ref ApiBridgeServerInfo server;
    ref array<ref ApiBridgePlayerState> players;
};

// Command queue --------------------------------------------------------------

class ApiBridgeCommand
{
    string id;
    string type; // teleport, inv_add, inv_remove, inv_setqty, stats_set
    string uid;

    // generic payload
    string itemType;
    int quantity;
    float health;

    float x;
    float y;
    float z;

    float setHealth;
    float setBlood;
    float setWater;
    float setEnergy;

    bool setHealthEnabled;
    bool setBloodEnabled;
    bool setWaterEnabled;
    bool setEnergyEnabled;

    int ts;
};

class ApiBridgeCommandBatch
{
    ref array<ref ApiBridgeCommand> commands;
};

class ApiBridgeResponse
{
    string id;
    bool ok;
    string message;
    int ts;
};

class ApiBridgeResponseBatch
{
    ref array<ref ApiBridgeResponse> responses;
};
