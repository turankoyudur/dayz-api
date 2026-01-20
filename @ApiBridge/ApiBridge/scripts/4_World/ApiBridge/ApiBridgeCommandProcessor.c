// ApiBridgeCommandProcessor.c (4_World)

class ApiBridgeCommandProcessor
{
    static string CommandsPath() { return "$profile:ApiBridge\\commands.json"; }
    static string ProcessingPath() { return "$profile:ApiBridge\\commands.processing.json"; }
    static string ResultsPath() { return "$profile:ApiBridge\\responses.json"; }

    static bool ClaimCommandsFile()
    {
        if (!FileExist(CommandsPath()))
            return false;

        // best effort atomic claim: copy then delete
        CopyFile(CommandsPath(), ProcessingPath());
        DeleteFile(CommandsPath());
        return FileExist(ProcessingPath());
    }

    static PlayerBase FindPlayerByUid(string uid)
    {
        if (uid == "") return null;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        foreach (Man m : players)
        {
            PlayerBase pb = PlayerBase.Cast(m);
            if (!pb) continue;
            PlayerIdentity id = pb.GetIdentity();
            if (!id) continue;
            if (id.GetId() == uid) return pb;
        }
        return null;
    }

    static EntityAI FindItemByType(PlayerBase player, string itemType)
    {
        if (!player || itemType == "") return null;

        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);
        foreach (EntityAI item : items)
        {
            if (!item) continue;
            if (item == player) continue;
            if (item.GetType() == itemType) return item;
        }
        return null;
    }

    static int RemoveItemsByType(PlayerBase player, string itemType, int count)
    {
        if (!player || itemType == "" || count <= 0) return 0;

        int removed = 0;
        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);
        foreach (EntityAI item : items)
        {
            if (removed >= count) break;
            if (!item) continue;
            if (item == player) continue;
            if (item.GetType() != itemType) continue;

            GetGame().ObjectDelete(item);
            removed++;
        }
        return removed;
    }

    static void SetItemQuantity(EntityAI item, float qty)
    {
        if (!item) return;

        Magazine mag = Magazine.Cast(item);
        if (mag)
        {
            int newCount = Math.Max(0, (int)qty);
            mag.ServerSetAmmoCount(newCount);
            return;
        }

        ItemBase ib = ItemBase.Cast(item);
        if (ib)
        {
            ib.SetQuantity(qty);
        }
    }

    static bool ApplyCommand(ApiBridgeCommand cmd, out string message)
    {
        message = "";
        if (!cmd) { message = "null cmd"; return false; }

        PlayerBase player = FindPlayerByUid(cmd.uid);
        if (!player) { message = "player not found"; return false; }

        if (cmd.type == "teleport")
        {
            vector p = Vector(cmd.x, cmd.y, cmd.z);
            player.SetPosition(p);
            message = "teleported";
            return true;
        }

        if (cmd.type == "inv_add")
        {
            EntityAI created = player.GetInventory().CreateInInventory(cmd.itemType);
            if (!created) { message = "create failed"; return false; }
            if (cmd.quantity > 0) SetItemQuantity(created, cmd.quantity);
            message = "item added";
            return true;
        }

        if (cmd.type == "inv_remove")
        {
            int removed = RemoveItemsByType(player, cmd.itemType, cmd.count);
            if (removed <= 0) { message = "no items removed"; return false; }
            message = "removed=" + removed.ToString();
            return true;
        }

        if (cmd.type == "inv_setqty")
        {
            EntityAI item = FindItemByType(player, cmd.itemType);
            if (!item) { message = "item not found"; return false; }
            SetItemQuantity(item, cmd.quantity);
            message = "quantity set";
            return true;
        }

        if (cmd.type == "stats_set")
        {
            // only set fields that are > 0 (or >=0 for water/energy)
            if (cmd.health > 0) player.SetHealth("", "", cmd.health);
            if (cmd.blood > 0)  player.SetHealth("", "Blood", cmd.blood);
            if (cmd.water >= 0 && player.GetStatWater()) player.GetStatWater().Set(cmd.water);
            if (cmd.energy >= 0 && player.GetStatEnergy()) player.GetStatEnergy().Set(cmd.energy);

            message = "stats updated";
            return true;
        }

        message = "unknown type: " + cmd.type;
        return false;
    }

    static void SaveResults(ApiBridgeResultBatch batch)
    {
        MakeDirectory("$profile:ApiBridge");
        JsonFileLoader<ApiBridgeResultBatch>.JsonSaveFile(ResultsPath(), batch);
    }

    static void ProcessOnce()
    {
        MakeDirectory("$profile:ApiBridge");

        if (!ClaimCommandsFile())
            return;

        ApiBridgeCommandBatch batch = new ApiBridgeCommandBatch();
        JsonFileLoader<ApiBridgeCommandBatch>.JsonLoadFile(ProcessingPath(), batch);
        DeleteFile(ProcessingPath());

        if (!batch || !batch.commands || batch.commands.Count() == 0)
            return;

        ApiBridgeResultBatch results = new ApiBridgeResultBatch();
        results.results = new array<ref ApiBridgeCommandResult>();

        foreach (ApiBridgeCommand cmd : batch.commands)
        {
            ApiBridgeCommandResult r = new ApiBridgeCommandResult();
            r.id = cmd ? cmd.id : "";

            string msg;
            bool success = ApplyCommand(cmd, msg);
            r.success = success;
            r.message = msg;

            results.results.Insert(r);
        }

        SaveResults(results);
    }
};
