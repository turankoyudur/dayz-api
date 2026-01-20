// Shared constants (available to all script modules)
static const string AB_PROFILE_DIR = "$profile:\\ApiBridge\\";
static const string AB_CFG_PATH    = AB_PROFILE_DIR + "apibridge.cfg";
static const string AB_STATE_PATH  = AB_PROFILE_DIR + "state.json";
static const string AB_PLAYERS_PATH = AB_PROFILE_DIR + "players.json";
static const string AB_COMMANDS_PATH = AB_PROFILE_DIR + "commands.json";
static const string AB_RESULTS_PATH = AB_PROFILE_DIR + "command_results.json";

// Heartbeat / link-check files used for Node <-> Mod connectivity monitoring
static const string AB_NODE_HB_PATH   = AB_PROFILE_DIR + "node_heartbeat.json";
static const string AB_BRIDGE_HB_PATH = AB_PROFILE_DIR + "bridge_heartbeat.json";
