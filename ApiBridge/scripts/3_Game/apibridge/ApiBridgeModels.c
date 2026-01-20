class ApiBridgePlayerSnapshot
{
    string name;
    string id;
    vector pos;
    float health;
};

class ApiBridgeStateSnapshot
{
    string world;
    int serverTimeMs;
    int playerCount;
    ref array<ref ApiBridgePlayerSnapshot> players;

    void ApiBridgeStateSnapshot()
    {
        players = new array<ref ApiBridgePlayerSnapshot>;
    }
};
