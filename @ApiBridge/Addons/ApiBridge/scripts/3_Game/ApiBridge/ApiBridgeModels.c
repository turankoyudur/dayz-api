// ApiBridgeModels.c

// ----- State -----
class ApiBridgeItemState
{
    string type;
    float quantity;
    float health;
    string location; // cargo / attachment / hands / unknown
};

class ApiBridgePlayerState
{
    string uid;
    string name;
    float x;
    float y;
    float z;

    float health;
    float blood;
    float water;
    float energy;

    ref array<ref ApiBridgeItemState> inventory;
};

class ApiBridgeState
{
    int ts;
    int playerCount;
    ref array<ref ApiBridgePlayerState> players;
};
