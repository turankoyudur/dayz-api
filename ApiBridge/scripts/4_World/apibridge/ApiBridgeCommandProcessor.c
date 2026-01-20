class ApiBridgeCommandProcessor
{
    ApiBridgeConfig m_Cfg;

    void ApiBridgeCommandProcessor(ApiBridgeConfig cfg)
    {
        m_Cfg = cfg;
    }

    void Tick()
    {
        ApiBridgeCommandBatch batch;
        if (!ApiBridgeFile.TryLoadCommands(batch))
            return;

        ApiBridgeCommandResults results = new ApiBridgeCommandResults();
        results.serverTimeMs = GetGame().GetTime();

        for (int i = 0; i < batch.commands.Count(); i++)
        {
            ApiBridgeCommand cmd = batch.commands.Get(i);
            ApiBridgeCommandResult r = new ApiBridgeCommandResult();
            r.id = "";
            r.ok = false;
            r.message = "";

            if (cmd)
            {
                r.id = cmd.id;
                HandleCommand(cmd, r);
            }
            else
            {
                r.message = "null command";
            }

            results.results.Insert(r);
        }

        ApiBridgeFile.SaveResults(results);
    }

    void HandleCommand(ApiBridgeCommand cmd, out ApiBridgeCommandResult r)
    {
        if (!cmd)
        {
            r.ok = false;
            r.message = "null";
            return;
        }

        if (cmd.apiKey != m_Cfg.ApiKey)
        {
            r.ok = false;
            r.message = "bad apiKey";
            return;
        }

        PlayerBase pb = FindPlayerById(cmd.playerId);
        if (!pb)
        {
            r.ok = false;
            r.message = "player not found";
            return;
        }

        string t = cmd.type;
        if (t == "teleport")
        {
            vector p = Vector(cmd.x, cmd.y, cmd.z);
            pb.SetPosition(p);
            r.ok = true;
            r.message = "ok";
            return;
        }

        if (t == "inv_add")
        {
            bool ok = AddItem(pb, cmd.itemType, cmd.quantity);
            r.ok = ok;
            if (ok) r.message = "ok";
            else r.message = "failed";
            return;
        }

        if (t == "inv_remove")
        {
            int removed = RemoveItems(pb, cmd.itemType, cmd.count);
            r.ok = removed > 0;
            r.message = "removed " + removed.ToString();
            return;
        }

        if (t == "inv_setqty")
        {
            bool ok2 = SetItemQuantity(pb, cmd.itemType, cmd.quantity);
            r.ok = ok2;
            if (ok2) r.message = "ok";
            else r.message = "not found";
            return;
        }

        if (t == "set_health")
        {
            pb.SetHealth("", "Health", cmd.value);
            r.ok = true;
            r.message = "ok";
            return;
        }

        if (t == "set_blood")
        {
            pb.SetHealth("", "Blood", cmd.value);
            r.ok = true;
            r.message = "ok";
            return;
        }

        if (t == "set_shock")
        {
            pb.SetHealth("", "Shock", cmd.value);
            r.ok = true;
            r.message = "ok";
            return;
        }

        r.ok = false;
        r.message = "unknown type";
    }

    PlayerBase FindPlayerById(string playerId)
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players.Get(i));
            if (!pb) continue;

            PlayerIdentity id = pb.GetIdentity();
            if (!id) continue;

            if (id.GetId() == playerId)
                return pb;
        }

        return null;
    }

    bool AddItem(PlayerBase pb, string typeName, float qty)
    {
        if (!pb) return false;
        if (typeName == "") return false;

        EntityAI e = pb.GetInventory().CreateInInventory(typeName);
        if (!e) return false;

        ItemBase ib = ItemBase.Cast(e);
        if (ib && qty > 0)
        {
            if (ib.HasQuantity())
                ib.SetQuantity(qty);
        }

        return true;
    }

    int RemoveItems(PlayerBase pb, string typeName, int count)
    {
        if (!pb) return 0;
        if (typeName == "") return 0;

        int target = count;
        if (target <= 0) target = 1;

        array<EntityAI> items = new array<EntityAI>();
        pb.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

        int removed = 0;
        for (int i = 0; i < items.Count(); i++)
        {
            EntityAI e = items.Get(i);
            if (!e) continue;
            if (e == pb) continue;

            if (e.GetType() == typeName)
            {
                GetGame().ObjectDelete(e);
                removed++;
                if (removed >= target)
                    break;
            }
        }

        return removed;
    }

    bool SetItemQuantity(PlayerBase pb, string typeName, float qty)
    {
        if (!pb) return false;
        if (typeName == "") return false;

        array<EntityAI> items = new array<EntityAI>();
        pb.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

        for (int i = 0; i < items.Count(); i++)
        {
            EntityAI e = items.Get(i);
            if (!e) continue;
            if (e == pb) continue;

            if (e.GetType() == typeName)
            {
                ItemBase ib = ItemBase.Cast(e);
                if (ib && ib.HasQuantity())
                {
                    ib.SetQuantity(qty);
                    return true;
                }
            }
        }

        return false;
    }
};
