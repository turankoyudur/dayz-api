class ApiBridgeServer
{
	protected static ref ApiBridgeServer s_Instance;

	protected ref ApiBridgeConfig m_Cfg;
	protected int m_LastStateWriteMs;
	protected int m_LastLinkWriteMs;
	protected bool m_Inited;

	static ApiBridgeServer Get()
	{
		if (!s_Instance)
			s_Instance = new ApiBridgeServer;
		return s_Instance;
	}

	void InitIfNeeded()
	{
		if (m_Inited) return;
		m_Inited = true;
		ApiBridgeIO.LoadConfig(m_Cfg);
		// Always write an initial state/link quickly so external tools know we're alive
		m_LastStateWriteMs = 0;
		m_LastLinkWriteMs = 0;
		Print("[ApiBridge] Init ok. Profile dir: " + ApiBridgeConst.PROFILE_DIR);
	}

	void Tick()
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		InitIfNeeded();

		int now = GetGame().GetTime();

		// 1) Periodic state.json
		if (now - m_LastStateWriteMs >= m_Cfg.StateWriteIntervalMs)
		{
			ApiBridgeServerState state = BuildState();
			ApiBridgeIO.SaveState(state);
			m_LastStateWriteMs = now;
		}

		// 2) Periodic link.json (Node heartbeat tracking)
		if (now - m_LastLinkWriteMs >= m_Cfg.LinkWriteIntervalMs)
		{
			ApiBridgeLinkState link = BuildLinkState();
			ApiBridgeIO.SaveLink(link);
			m_LastLinkWriteMs = now;
		}

		// 3) Commands processing
		ApiBridgeCommandEnvelope env;
		if (ApiBridgeIO.LoadCommands(env))
		{
			ProcessCommands(env);
			ApiBridgeIO.AckCommands();
		}
	}

	ApiBridgeServerState BuildState()
	{
		ApiBridgeServerState state = new ApiBridgeServerState;
		state.serverTimeMs = GetGame().GetTime();
		state.worldName = GetGame().GetWorldName();

		array<Man> men = new array<Man>;
		GetGame().GetPlayers(men);
		state.playerCount = men.Count();

		foreach (Man m : men)
		{
			PlayerBase p = PlayerBase.Cast(m);
			if (!p) continue;
			PlayerIdentity ident = p.GetIdentity();
			if (!ident) continue;

			ApiBridgePlayerState ps = new ApiBridgePlayerState;
			ps.id = ident.GetId();
			ps.plainId = ident.GetPlainId();
			ps.name = ident.GetName();
			ps.pos = p.GetPosition();
			ps.health = p.GetHealth(); // global health
			state.players.Insert(ps);
		}

		return state;
	}

	ApiBridgeLinkState BuildLinkState()
	{
		ApiBridgeLinkState link = new ApiBridgeLinkState;
		link.serverTimeMs = GetGame().GetTime();

		string nonce;
		int t;
		if (ApiBridgeIO.LoadNodeHeartbeat(nonce, t))
		{
			link.lastNodeHeartbeatMs = t;
			link.lastNodeNonce = nonce;
			link.status = "linked";
		}
		else
		{
			link.status = "waiting_node";
		}

		return link;
	}

	void ProcessCommands(ApiBridgeCommandEnvelope env)
	{
		ApiBridgeResultsEnvelope outRes = new ApiBridgeResultsEnvelope;
		outRes.serverTimeMs = GetGame().GetTime();

		if (!env)
		{
			outRes.results.Insert(MakeErr("", "bad_envelope"));
			ApiBridgeIO.SaveResults(outRes);
			return;
		}

		// Version check (soft)
		if (env.apiVersion != ApiBridgeConst.API_VERSION)
		{
			outRes.results.Insert(MakeErr("", "api_version_mismatch"));
			ApiBridgeIO.SaveResults(outRes);
			return;
		}

		// API key check
		if (!m_Cfg || env.apiKey != m_Cfg.ApiKey)
		{
			outRes.results.Insert(MakeErr("", "auth_failed"));
			ApiBridgeIO.SaveResults(outRes);
			return;
		}

		if (!env.commands)
		{
			outRes.results.Insert(MakeErr("", "no_commands"));
			ApiBridgeIO.SaveResults(outRes);
			return;
		}

		foreach (ApiBridgeCommand cmd : env.commands)
		{
			outRes.results.Insert(HandleCommand(cmd));
		}

		ApiBridgeIO.SaveResults(outRes);
	}

	ApiBridgeCommandResult HandleCommand(ApiBridgeCommand cmd)
	{
		if (!cmd)
			return MakeErr("", "null_command");

		string t = cmd.type;
		if (t == "ping")
			return MakeOk(cmd.id, "pong");

		if (t == "server.status")
		{
			ApiBridgeCommandResult r = MakeOk(cmd.id, "ok");
			r.serverState = BuildState();
			return r;
		}

		if (t == "server.message")
		{
			BroadcastMessage(cmd.message);
			return MakeOk(cmd.id, "sent");
		}

		if (t == "player.kick")
		{
			if (!m_Cfg.EnableKick)
				return MakeErr(cmd.id, "kick_disabled");

			bool ok = KickByPlainId(cmd.playerPlainId);
			return ok ? MakeOk(cmd.id, "kicked") : MakeErr(cmd.id, "player_not_found");
		}

		if (t == "ban.add")
		{
			bool okAdd = AddBan(cmd.playerPlainId);
			if (okAdd)
			{
				KickByPlainId(cmd.playerPlainId);
				return MakeOk(cmd.id, "banned");
			}
			return MakeErr(cmd.id, "ban_failed");
		}

		if (t == "ban.remove")
		{
			bool okRem = RemoveBan(cmd.playerPlainId);
			return okRem ? MakeOk(cmd.id, "unbanned") : MakeErr(cmd.id, "not_in_banlist");
		}

		if (t == "server.restart")
		{
			if (!m_Cfg.EnableRestart)
				return MakeErr(cmd.id, "restart_disabled");
			GetGame().RequestRestart(0);
			return MakeOk(cmd.id, "restart_requested");
		}

		if (t == "server.shutdown")
		{
			if (!m_Cfg.EnableShutdown)
				return MakeErr(cmd.id, "shutdown_disabled");
			GetGame().RequestExit(0);
			return MakeOk(cmd.id, "shutdown_requested");
		}

		return MakeErr(cmd.id, "unknown_command");
	}

	ApiBridgeCommandResult MakeOk(string id, string data)
	{
		ApiBridgeCommandResult r = new ApiBridgeCommandResult;
		r.id = id;
		r.ok = true;
		r.data = data;
		r.error = "";
		return r;
	}

	ApiBridgeCommandResult MakeErr(string id, string err)
	{
		ApiBridgeCommandResult r = new ApiBridgeCommandResult;
		r.id = id;
		r.ok = false;
		r.error = err;
		r.data = "";
		return r;
	}

	void BroadcastMessage(string msg)
	{
		if (msg == string.Empty)
			return;

		array<Man> men = new array<Man>;
		GetGame().GetPlayers(men);
		foreach (Man m : men)
		{
			PlayerBase p = PlayerBase.Cast(m);
			if (!p) continue;
			PlayerIdentity ident = p.GetIdentity();
			if (!ident) continue;

			// RPC_USER_ACTION_MESSAGE is used by the vanilla action-message UI.
			Param1<string> pMsg = new Param1<string>(msg);
			GetGame().RPCSingleParam(p, ERPCs.RPC_USER_ACTION_MESSAGE, pMsg, true, ident);
		}
	}

	bool KickByPlainId(string plainId)
	{
		if (plainId == string.Empty)
			return false;

		array<Man> men = new array<Man>;
		GetGame().GetPlayers(men);
		foreach (Man m : men)
		{
			PlayerBase p = PlayerBase.Cast(m);
			if (!p) continue;
			PlayerIdentity ident = p.GetIdentity();
			if (!ident) continue;
			if (ident.GetPlainId() == plainId)
			{
				GetGame().DisconnectPlayer(ident);
				return true;
			}
		}
		return false;
	}

	bool AddBan(string plainId)
	{
		if (plainId == string.Empty) return false;
		if (!m_Cfg || !m_Cfg.BanPlainIds) return false;
		if (m_Cfg.BanPlainIds.Find(plainId) != -1) return true;
		m_Cfg.BanPlainIds.Insert(plainId);
		ApiBridgeIO.SaveConfig(m_Cfg);
		return true;
	}

	bool RemoveBan(string plainId)
	{
		if (plainId == string.Empty) return false;
		if (!m_Cfg || !m_Cfg.BanPlainIds) return false;
		int idx = m_Cfg.BanPlainIds.Find(plainId);
		if (idx == -1) return false;
		m_Cfg.BanPlainIds.Remove(idx);
		ApiBridgeIO.SaveConfig(m_Cfg);
		return true;
	}

	bool IsBanned(PlayerIdentity ident)
	{
		if (!m_Cfg || !m_Cfg.BanPlainIds || !ident) return false;
		return m_Cfg.BanPlainIds.Find(ident.GetPlainId()) != -1;
	}
};
