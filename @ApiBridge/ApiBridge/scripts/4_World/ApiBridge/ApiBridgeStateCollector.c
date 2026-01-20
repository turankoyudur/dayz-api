// ApiBridgeStateCollector.c (4_World)

class ApiBridgeStateCollector
{
    static string StatePath()
    {
        return "$profile:ApiBridge\\state.json";
    }

    static float GetItemQuantity(EntityAI item)
    {
        Magazine mag = Magazine.Cast(item);
        if (mag)
            return mag.GetAmmoCount();

        ItemBase ib = ItemBase.Cast(item);
        if (ib)
            return ib.GetQuantity();

        return 0;
    }

    static string GetItemLocation(PlayerBase player, EntityAI item)
    {
        // hands
        EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
        if (inHands && inHands == item)
            return "hands";

        // attached or cargo
        EntityAI parent = EntityAI.Cast(item.GetHierarchyParent());
        if (parent && parent == player)
        {
            // direct child (attachments)
            return "attached";
        }

        if (parent)
            return "cargo";

        return "ground";
    }

    static ApiBridgePlayerStats CollectStats(PlayerBase player)
    {
        ApiBridgePlayerStats s = new ApiBridgePlayerStats();

        // health / blood as damage system values
        s.health = player.GetHealth("", "");
        s.blood  = player.GetHealth("", "Blood");

        // water/energy via player stats (works on current DayZ)
        if (player.GetStatWater())
            s.water = player.GetStatWater().Get();
        if (player.GetStatEnergy())
            s.energy = player.GetStatEnergy().Get();

        return s;
    }

    static ApiBridgePlayerState CollectPlayer(PlayerBase player)
    {
        PlayerIdentity ident = player.GetIdentity();
        if (!ident)
            return null;

        ApiBridgePlayerState ps = new ApiBridgePlayerState();
        ps.uid = ident.GetId();
        ps.name = ident.GetName();

        vector p = player.GetPosition();
        ps.x = p[0];
        ps.y = p[1];
        ps.z = p[2];

        ps.stats = CollectStats(player);
        ps.inventory = new array<ref ApiBridgeItemState>();

        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);

        foreach (EntityAI item : items)
        {
            if (!item) continue;
            if (item == player) continue;

            ApiBridgeItemState it = new ApiBridgeItemState();
            it.type = item.GetType();
            it.health = item.GetHealth("", "");
            it.quantity = GetItemQuantity(item);

            EntityAI parent = EntityAI.Cast(item.GetHierarchyParent());
            it.parentType = parent ? parent.GetType() : "";
            it.location = GetItemLocation(player, item);

            ps.inventory.Insert(it);
        }

        return ps;
    }

    static void CollectAndSave(float uptimeSeconds)
    {
        MakeDirectory("$profile:ApiBridge");

        ApiBridgeServerState s = new ApiBridgeServerState();
        int year, month, day, hour, minute;
        GetGame().GetWorld().GetDate(year, month, day, hour, minute);
        s.serverTime = year.ToString() + "-" + month.ToString() + "-" + day.ToString() + " " + hour.ToString() + ":" + minute.ToString();
        s.uptimeSeconds = uptimeSeconds;
        s.players = new array<ref ApiBridgePlayerState>();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        foreach (Man man : players)
        {
            PlayerBase pb = PlayerBase.Cast(man);
            if (!pb) continue;

            ApiBridgePlayerState ps = CollectPlayer(pb);
            if (ps)
                s.players.Insert(ps);
        }

        s.playerCount = s.players.Count();

        JsonFileLoader<ApiBridgeServerState>.JsonSaveFile(StatePath(), s);
    }
};
