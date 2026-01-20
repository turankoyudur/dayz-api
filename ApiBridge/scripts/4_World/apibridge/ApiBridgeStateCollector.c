class ApiBridgeStateCollector
{
    ApiBridgeConfig m_Cfg;

    void ApiBridgeStateCollector(ApiBridgeConfig cfg)
    {
        m_Cfg = cfg;
    }

    ApiBridgeStateSnapshot Collect()
    {
        ApiBridgeStateSnapshot state = new ApiBridgeStateSnapshot();
        state.world = GetGame().GetWorldName();
        state.serverTimeMs = GetGame().GetTime();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        state.playerCount = players.Count();

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players.Get(i));
            if (!pb) continue;

            ApiBridgePlayerSnapshot ps = BuildPlayerSnapshot(pb);
            state.players.Insert(ps);
        }

        return state;
    }

    ApiBridgePlayerSnapshot BuildPlayerSnapshot(PlayerBase pb)
    {
        ApiBridgePlayerSnapshot ps = new ApiBridgePlayerSnapshot();
        ps.pos = pb.GetPosition();

        // These component names are stable.
        ps.health = pb.GetHealth("", "Health");
        ps.blood = pb.GetHealth("", "Blood");
        ps.shock = pb.GetHealth("", "Shock");

        PlayerIdentity id = pb.GetIdentity();
        if (id)
        {
            ps.name = id.GetName();
            ps.id = id.GetId();
        }
        else
        {
            ps.name = "<noid>";
            ps.id = "";
        }

        FillInventory(pb, ps);
        return ps;
    }

    void FillInventory(PlayerBase pb, out ApiBridgePlayerSnapshot ps)
    {
        array<EntityAI> items = new array<EntityAI>();
        pb.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

        int maxItems = m_Cfg.MaxItemsPerPlayer;
        if (maxItems <= 0) maxItems = 200;

        EntityAI inHands = pb.GetItemInHands();

        int count = 0;
        for (int i = 0; i < items.Count(); i++)
        {
            EntityAI e = items.Get(i);
            if (!e) continue;
            if (e == pb) continue;

            ApiBridgeItemSnapshot it = new ApiBridgeItemSnapshot();
            it.type = e.GetType();
            it.health = e.GetHealth("", "Health");

            it.quantity = -1;
            ItemBase ib = ItemBase.Cast(e);
            if (ib && ib.HasQuantity())
                it.quantity = ib.GetQuantity();

            EntityAI parent = e.GetHierarchyParent();
            if (parent)
                it.parentType = parent.GetType();
            else
                it.parentType = "";

            it.location = "Inventory";
            if (!parent)
                it.location = "Ground";

            if (inHands && e == inHands)
                it.location = "Hands";

            ps.inventory.Insert(it);
            count++;
            if (count >= maxItems)
                break;
        }

        ps.itemCount = count;
    }
};
