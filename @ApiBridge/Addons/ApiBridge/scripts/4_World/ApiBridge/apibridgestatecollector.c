// apibridgestatecollector.c (4_World)

class ApiBridgeStateCollector
{
    static string StatePath() { return "$profile:ApiBridge/state.json"; }

    static void WriteState()
    {
        ApiBridgeConfig.EnsureDir();

        ref ApiBridgeState state = new ApiBridgeState();
        state.ts = GetGame().GetTime();
        state.players = new array<ref ApiBridgePlayerState>();

        ref array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        state.playerCount = players.Count();

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players.Get(i));
            if (!pb) continue;

            PlayerIdentity id = pb.GetIdentity();
            if (!id) continue;

            ref ApiBridgePlayerState ps = new ApiBridgePlayerState();
            ps.uid = id.GetId();
            ps.name = id.GetName();

            vector p = pb.GetPosition();
            ps.x = p[0];
            ps.y = p[1];
            ps.z = p[2];

            // Stats (these accessors exist on modern DayZ; remove if your server build differs)
            ps.health = pb.GetHealth("", "Health");
            ps.blood = pb.GetHealth("", "Blood");

            if (pb.GetStatWater()) ps.water = pb.GetStatWater().Get();
            if (pb.GetStatEnergy()) ps.energy = pb.GetStatEnergy().Get();

            ps.inventory = CollectInventory(pb);

            state.players.Insert(ps);
        }

        JsonFileLoader<ApiBridgeState>.JsonSaveFile(StatePath(), state);
    }

    private static ref array<ref ApiBridgeItemState> CollectInventory(PlayerBase pb)
    {
        ref array<ref ApiBridgeItemState> outInv = new array<ref ApiBridgeItemState>();

        ref array<EntityAI> ents = new array<EntityAI>();
        pb.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, ents);

        EntityAI hands = null;
        HumanInventory hi = pb.GetHumanInventory();
        if (hi) hands = hi.GetEntityInHands();

        for (int j = 0; j < ents.Count(); j++)
        {
            EntityAI e = ents.Get(j);
            if (!e || e == pb) continue;

            ref ApiBridgeItemState it = new ApiBridgeItemState();
            it.type = e.GetType();
            it.health = e.GetHealth("", "");
            it.location = "unknown";

            if (hands && e == hands) it.location = "hands";

            ItemBase ib = ItemBase.Cast(e);
            if (ib) it.quantity = ib.GetQuantity();
            else it.quantity = -1;

            outInv.Insert(it);
        }

        return outInv;
    }
}
