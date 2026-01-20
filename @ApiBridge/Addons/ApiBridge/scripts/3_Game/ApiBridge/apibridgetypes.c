// apibridgetypes.c (3_Game)

// ----- Commands -----
class ApiBridgeCommand
{
    string id;
    string uid;
    string type;

    // teleport
    float x;
    float y;
    float z;

    // inventory
    string itemType;
    int quantity;
    float health;

    // stats
    bool setHealthEnabled;
    float setHealth;

    bool setBloodEnabled;
    float setBlood;

    bool setWaterEnabled;
    float setWater;

    bool setEnergyEnabled;
    float setEnergy;
}

class ApiBridgeCommandBatch
{
    ref array<ref ApiBridgeCommand> commands;
}

// ----- Responses -----
class ApiBridgeResponse
{
    string id;
    bool ok;
    int ts;
    string message;
}

class ApiBridgeResponseBatch
{
    ref array<ref ApiBridgeResponse> responses;
}

// ----- State -----
// State models are defined in ApiBridgeModels.c to avoid duplicate declarations.
