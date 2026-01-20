// apibridgecommandprocessor.c (4_World)

class ApiBridgeCommandProcessor
{
    static string CommandsPath() { return "$profile:ApiBridge/commands.json"; }
    static string ProcessingPath() { return "$profile:ApiBridge/commands.processing.json"; }
    static string ResponsesPath() { return "$profile:ApiBridge/responses.json"; }

    static void TickProcessCommands()
    {
        ApiBridgeConfig.EnsureDir();
        if (!FileExist(CommandsPath())) return;

        // best-effort atomic handoff: copy -> delete -> read copy
        CopyFile(CommandsPath(), ProcessingPath());
        DeleteFile(CommandsPath());

        ref ApiBridgeCommandBatch batch = new ApiBridgeCommandBatch();
        batch.commands = new array<ref ApiBridgeCommand>();

        // Compatibility: JsonLoadFile may return void in some script versions.
        JsonFileLoader<ApiBridgeCommandBatch>.JsonLoadFile(ProcessingPath(), batch);
        DeleteFile(ProcessingPath());
        if (!batch || !batch.commands || batch.commands.Count() == 0) return;

        for (int i = 0; i < batch.commands.Count(); i++)
        {
            ApiBridgeCommand cmd = batch.commands.Get(i);
            if (!cmd) continue;
            ApiBridgeResponse resp = Execute(cmd);
            AppendResponse(resp);
        }
    }

    static ApiBridgeResponse Execute(ApiBridgeCommand cmd)
    {
        ref ApiBridgeResponse r = new ApiBridgeResponse();
        r.id = cmd.id;
        r.ok = false;
        r.ts = GetGame().GetTime();
        r.message = "unknown error";

        PlayerBase pb = FindPlayerByUid(cmd.uid);
        if (!pb)
        {
            r.message = "player not found";
            return r;
        }

        string t = cmd.type;

        if (t == "teleport")
        {
            pb.SetPosition(Vector(cmd.x, cmd.y, cmd.z));
            r.ok = true;
            r.message = "teleported";
            return r;
        }

        if (t == "inv_add")
        {
            if (cmd.itemType == "") { r.message = "itemType missing"; return r; }

            EntityAI e = pb.GetInventory().CreateInInventory(cmd.itemType);
            if (!e)
            {
                // fallback: try hands if inventory is full
                HumanInventory hi = pb.GetHumanInventory();
                if (hi) e = hi.CreateInHands(cmd.itemType);
            }

            if (!e)
            {
                r.message = "could not create item (no space?)";
                return r;
            }

            ItemBase ib = ItemBase.Cast(e);
            if (ib && cmd.quantity > 0) ib.SetQuantity(cmd.quantity);
            if (cmd.health > 0) e.SetHealth("", "", cmd.health);

            r.ok = true;
            r.message = "item added";
            return r;
        }

        if (t == "inv_remove")
        {
            if (cmd.itemType == "") { r.message = "itemType missing"; return r; }

            int toRemove = cmd.quantity;
            if (toRemove <= 0) toRemove = 1;

            ref array<EntityAI> ents = new array<EntityAI>();
            pb.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, ents);

            int removed = 0;
            for (int j = 0; j < ents.Count() && removed < toRemove; j++)
            {
                EntityAI e2 = ents.Get(j);
                if (!e2 || e2 == pb) continue;
                if (e2.GetType() != cmd.itemType) continue;

                GetGame().ObjectDelete(e2);
                removed++;
            }

            if (removed == 0)
            {
                r.message = "no matching items";
                return r;
            }

            r.ok = true;
            r.message = "removed " + removed.ToString();
            return r;
        }

        if (t == "inv_setqty")
        {
            if (cmd.itemType == "") { r.message = "itemType missing"; return r; }
            if (cmd.quantity < 0) { r.message = "quantity missing"; return r; }

            ref array<EntityAI> ents2 = new array<EntityAI>();
            pb.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, ents2);

            for (int k = 0; k < ents2.Count(); k++)
            {
                EntityAI e3 = ents2.Get(k);
                if (!e3 || e3 == pb) continue;
                if (e3.GetType() != cmd.itemType) continue;

                ItemBase ib2 = ItemBase.Cast(e3);
                if (!ib2)
                {
                    r.message = "item is not ItemBase";
                    return r;
                }

                ib2.SetQuantity(cmd.quantity);
                r.ok = true;
                r.message = "quantity set";
                return r;
            }

            r.message = "no matching item";
            return r;
        }

        if (t == "stats_set")
        {
            if (cmd.setHealthEnabled) pb.SetHealth("", "", cmd.setHealth);
            if (cmd.setBloodEnabled) pb.SetHealth("", "Blood", cmd.setBlood);

            if (cmd.setWaterEnabled && pb.GetStatWater()) pb.GetStatWater().Set(cmd.setWater);
            if (cmd.setEnergyEnabled && pb.GetStatEnergy()) pb.GetStatEnergy().Set(cmd.setEnergy);

            r.ok = true;
            r.message = "stats updated";
            return r;
        }

        r.message = "unknown command type: " + t;
        return r;
    }

    static PlayerBase FindPlayerByUid(string uid)
    {
        ref array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players.Get(i));
            if (!pb) continue;
            PlayerIdentity id = pb.GetIdentity();
            if (!id) continue;
            if (id.GetId() == uid) return pb;
        }
        return null;
    }

    static void AppendResponse(ApiBridgeResponse r)
    {
        if (!r) return;

        ref ApiBridgeResponseBatch b = new ApiBridgeResponseBatch();
        b.responses = new array<ref ApiBridgeResponse>();

        if (FileExist(ResponsesPath()))
        {
            JsonFileLoader<ApiBridgeResponseBatch>.JsonLoadFile(ResponsesPath(), b);
            if (!b.responses) b.responses = new array<ref ApiBridgeResponse>();
        }

        b.responses.Insert(r);

        // keep last 1000 responses
        while (b.responses.Count() > 1000)
        {
            b.responses.Remove(0);
        }

        JsonFileLoader<ApiBridgeResponseBatch>.JsonSaveFile(ResponsesPath(), b);
    }
};
