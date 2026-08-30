#pragma once

class PlayerDI;
extern std::atomic<bool> DupeItems;
extern std::atomic<bool> RapidFire;
extern std::atomic<bool> InfiniteAmmo;

namespace WebGUI
{
    template <typename T, uint32_t Capacity>
    class SPSCQueue
    {
        static_assert(Capacity >= 2);
        static_assert(std::is_trivially_copyable_v<T>);

    public:
        bool Push(const T& Value)
        {
            const uint32_t CurrentHead = Head.load(std::memory_order_relaxed);
            const uint32_t NextHead = Increment(CurrentHead);

            if (NextHead == Tail.load(std::memory_order_acquire))
                return false;

            Data[CurrentHead] = Value;
            Head.store(NextHead, std::memory_order_release);
            return true;
        }

        bool Pop(T& Value)
        {
            const uint32_t CurrentTail = Tail.load(std::memory_order_relaxed);

            if (CurrentTail == Head.load(std::memory_order_acquire))
                return false;

            Value = Data[CurrentTail];
            Tail.store(Increment(CurrentTail), std::memory_order_release);
            return true;
        }

    private:
        static constexpr uint32_t Increment(uint32_t Index)
        {
            return Index + 1 == Capacity ? 0 : Index + 1;
        }

        alignas(64) std::array<T, Capacity> Data{};
        alignas(64) std::atomic<uint32_t> Head{};
        alignas(64) std::atomic<uint32_t> Tail{};
    };

    enum class CommandType : uint8_t
    {
        SpawnItem,
        SpawnEntity,
        SpawnExplosion,
        TeleportToPlayer,
    };

    struct Command
    {
        CommandType Type{};
        uintptr_t Target{};
        int32_t ID{};
        int32_t Quantity{ 1 };
        char Name[256]{};
    };

    inline constexpr uint32_t MaximumPlayers = 64;
    inline constexpr uintptr_t PlayerControlObjectOffset = 0x18;

    struct PlayerList
    {
        uintptr_t Players[MaximumPlayers]{};
        uint32_t Count{};
        uintptr_t LocalPlayer{};
    };

    class SharedPlayerList
    {
    public:
        void Write(const uintptr_t* Data, uint32_t Count, uintptr_t LocalPlayer)
        {
            Count = Count > MaximumPlayers ? MaximumPlayers : Count;
            Sequence.fetch_add(1, std::memory_order_acq_rel);

            for (uint32_t Index = 0; Index < Count; ++Index)
                Players[Index].store(Data[Index], std::memory_order_relaxed);

            this->LocalPlayer.store(LocalPlayer, std::memory_order_relaxed);
            this->Count.store(Count, std::memory_order_relaxed);
            Sequence.fetch_add(1, std::memory_order_release);
        }

        PlayerList Read() const
        {
            PlayerList Result{};

            for (;;)
            {
                const uint64_t Before = Sequence.load(std::memory_order_acquire);
                if (Before & 1)
                    continue;

                Result.Count = Count.load(std::memory_order_relaxed);
                Result.LocalPlayer = LocalPlayer.load(std::memory_order_relaxed);

                for (uint32_t Index = 0; Index < Result.Count; ++Index)
                    Result.Players[Index] = Players[Index].load(std::memory_order_relaxed);

                if (Before == Sequence.load(std::memory_order_acquire))
                    return Result;
            }
        }

    private:
        alignas(64) mutable std::atomic<uint64_t> Sequence{};
        alignas(64) std::array<std::atomic<uintptr_t>, MaximumPlayers> Players{};
        std::atomic<uint32_t> Count{};
        std::atomic<uintptr_t> LocalPlayer{};
    };

    inline constexpr int Port = 31847;

    inline SPSCQueue<Command, 256> Commands;
    inline httplib::Server Server;
    inline std::thread ServerThread;
    inline std::atomic<int> ServerState{};
    inline std::atomic_bool Configured{};
    inline SharedPlayerList ActivePlayers;
    inline std::atomic<uint64_t> LastPlayerUpdate{};

    template <typename T>
    inline void UpdatePlayers(const T& Players, PlayerDI* LocalPlayer)
    {
        uintptr_t Snapshot[MaximumPlayers]{};
        const uintptr_t Local = reinterpret_cast<uintptr_t>(LocalPlayer);
        bool ControlObjectPointers = false;
        uint32_t Count = 0;

        for (uint32_t Index = 0; Local && Index < Players.Count; ++Index)
        {
            if (Players.Data[Index] == Local + PlayerControlObjectOffset)
            {
                ControlObjectPointers = true;
                break;
            }
        }

        if (Local)
            Snapshot[Count++] = Local;

        for (uint32_t Index = 0; Index < Players.Count && Count < MaximumPlayers; ++Index)
        {
            const uintptr_t RawPlayer = Players.Data[Index];
            if (!RawPlayer)
                continue;

            const uintptr_t Player = RawPlayer - (ControlObjectPointers ? PlayerControlObjectOffset : 0);
            if (Player && Player != Local)
                Snapshot[Count++] = Player;
        }

        ActivePlayers.Write(Snapshot, Count, Local);
        LastPlayerUpdate.store(GetTickCount64(), std::memory_order_release);
    }

    inline void ClearPlayers()
    {
        ActivePlayers.Write(nullptr, 0, 0);
        LastPlayerUpdate.store(0, std::memory_order_release);
    }

    inline PlayerList ReadActivePlayers()
    {
        const uint64_t Updated = LastPlayerUpdate.load(std::memory_order_acquire);
        return Updated && GetTickCount64() - Updated <= 2000 ? ActivePlayers.Read() : PlayerList{};
    }

    inline bool IsActivePlayer(uintptr_t Player)
    {
        if (!Player)
            return false;

        const PlayerList List = ReadActivePlayers();

        for (uint32_t Index = 0; Index < List.Count; ++Index)
        {
            if (List.Players[Index] == Player)
                return true;
        }

        return false;
    }

    template <typename T>
    inline PlayerDI* ResolveTarget(const Command& Value, const T& CurrentPlayers, PlayerDI* LocalPlayer)
    {
        const uintptr_t Requested = Value.Target ? Value.Target : reinterpret_cast<uintptr_t>(LocalPlayer);
        const uintptr_t Local = reinterpret_cast<uintptr_t>(LocalPlayer);
        bool ControlObjectPointers = false;

        if (!Requested)
            return nullptr;

        if (Requested == Local)
            return LocalPlayer;

        for (uint32_t Index = 0; Local && Index < CurrentPlayers.Count; ++Index)
        {
            if (CurrentPlayers.Data[Index] == Local + PlayerControlObjectOffset)
            {
                ControlObjectPointers = true;
                break;
            }
        }

        for (uint32_t Index = 0; Index < CurrentPlayers.Count; ++Index)
        {
            const uintptr_t RawPlayer = CurrentPlayers.Data[Index];
            if (!RawPlayer)
                continue;

            const uintptr_t Player = RawPlayer - (ControlObjectPointers ? PlayerControlObjectOffset : 0);
            if (Player == Requested)
                return reinterpret_cast<PlayerDI*>(Requested);
        }

        return nullptr;
    }

    inline constexpr const char* Page = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Dying Light Spawner</title>
<style>
    :root {
        --bg: #0b0d10;
        --panel: #14171c;
        --panel-alt: #191d23;
        --border: #262b33;
        --border-soft: #1e222a;
        --text: #eef0f3;
        --text-dim: #8b93a1;
        --accent: #ff4d4d;
        --accent-hover: #ff6666;
        --accent-dim: rgba(255, 77, 77, 0.12);
        --ok: #35d68a;
        --err: #ff5c5c;
        --radius: 14px;
        --radius-sm: 9px;
    }

    * {
        box-sizing: border-box;
    }

    body {
        margin: 0;
        min-height: 100vh;
        background:
            radial-gradient(1200px 600px at 15% -10%, rgba(255, 77, 77, 0.08), transparent 60%),
            radial-gradient(900px 500px at 100% 0%, rgba(53, 214, 138, 0.05), transparent 55%),
            var(--bg);
        color: var(--text);
        font: 15px/1.5 "Segoe UI", system-ui, -apple-system, Arial, sans-serif;
        -webkit-font-smoothing: antialiased;
    }

    main {
        width: min(640px, calc(100% - 32px));
        margin: 48px auto 80px;
    }

    header {
        display: flex;
        align-items: center;
        gap: 12px;
        margin-bottom: 28px;
    }

    .logo-dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
        background: var(--accent);
        box-shadow: 0 0 14px 2px var(--accent);
        flex-shrink: 0;
    }

    header h1 {
        margin: 0;
        font-size: 21px;
        font-weight: 700;
        letter-spacing: 0.2px;
    }

    header .tag {
        margin-left: auto;
        font-size: 11px;
        font-weight: 600;
        letter-spacing: 0.6px;
        text-transform: uppercase;
        color: var(--text-dim);
        border: 1px solid var(--border);
        padding: 4px 10px;
        border-radius: 999px;
        background: var(--panel-alt);
    }

    .card {
        background: linear-gradient(180deg, var(--panel) 0%, var(--panel-alt) 100%);
        border: 1px solid var(--border);
        border-radius: var(--radius);
        padding: 22px;
        margin-bottom: 18px;
        box-shadow: 0 8px 24px -12px rgba(0, 0, 0, 0.6);
    }

    .card h2 {
        margin: 0 0 4px;
        font-size: 14px;
        font-weight: 700;
        text-transform: uppercase;
        letter-spacing: 0.8px;
        color: var(--text-dim);
    }

    .card p.desc {
        margin: 0 0 16px;
        font-size: 12.5px;
        color: var(--text-dim);
    }

    .field-label {
        display: block;
        font-size: 12px;
        font-weight: 600;
        color: var(--text-dim);
        margin: 14px 0 6px;
    }

    .field-label:first-child {
        margin-top: 0;
    }

    input, select {
        width: 100%;
        padding: 11px 13px;
        border: 1px solid var(--border);
        border-radius: var(--radius-sm);
        background: #0f1216;
        color: var(--text);
        font-size: 14px;
        transition: border-color 0.15s ease, box-shadow 0.15s ease;
    }

    input::placeholder {
        color: #55606e;
    }

    input:focus, select:focus {
        outline: none;
        border-color: var(--accent);
        box-shadow: 0 0 0 3px var(--accent-dim);
    }

    select:disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }

    .row {
        display: flex;
        gap: 12px;
    }

    .row > div {
        flex: 1;
    }

    button {
        width: 100%;
        margin-top: 16px;
        padding: 12px;
        border: none;
        border-radius: var(--radius-sm);
        background: var(--accent);
        color: #1a0505;
        font-size: 14px;
        font-weight: 700;
        letter-spacing: 0.2px;
        cursor: pointer;
        transition: background 0.15s ease, transform 0.05s ease;
    }

    button:hover {
        background: var(--accent-hover);
    }

    button:active {
        transform: scale(0.99);
    }

    button:disabled {
        cursor: wait;
        opacity: 0.55;
    }

    button.secondary {
        background: transparent;
        color: var(--text);
        border: 1px solid var(--border);
    }

    button.secondary:hover {
        background: var(--panel-alt);
        border-color: #3a4048;
    }

    .toggle-row {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        padding: 13px 16px;
        border: 1px solid var(--border);
        border-radius: var(--radius-sm);
        background: #0f1216;
    }

    .toggle-row + .toggle-row {
        margin-top: 10px;
    }

    .toggle-row span {
        font-size: 13.5px;
        font-weight: 500;
    }

    .switch {
        position: relative;
        width: 40px;
        height: 22px;
        flex-shrink: 0;
    }

    .switch input {
        opacity: 0;
        width: 0;
        height: 0;
        position: absolute;
    }

    .switch .slider {
        position: absolute;
        inset: 0;
        background: #2a2f37;
        border-radius: 999px;
        cursor: pointer;
        transition: background 0.15s ease;
    }

    .switch .slider::before {
        content: "";
        position: absolute;
        width: 16px;
        height: 16px;
        left: 3px;
        top: 3px;
        background: #cfd3d9;
        border-radius: 50%;
        transition: transform 0.15s ease, background 0.15s ease;
    }

    .switch input:checked + .slider {
        background: var(--accent);
    }

    .switch input:checked + .slider::before {
        transform: translateX(18px);
        background: #fff;
    }

    #player-status {
        margin-top: 8px;
        color: var(--text-dim);
        font-size: 12.5px;
        display: flex;
        align-items: center;
        gap: 6px;
    }

    #player-status .pulse {
        width: 6px;
        height: 6px;
        border-radius: 50%;
        background: var(--text-dim);
        flex-shrink: 0;
    }

    #player-status.live .pulse {
        background: var(--ok);
        box-shadow: 0 0 6px 1px var(--ok);
    }

    #status-bar {
        position: fixed;
        left: 50%;
        bottom: 24px;
        transform: translateX(-50%);
        min-width: 240px;
        max-width: min(90%, 480px);
        padding: 12px 18px;
        border-radius: 999px;
        background: #10141a;
        border: 1px solid var(--border);
        color: var(--ok);
        font-size: 13px;
        font-weight: 600;
        text-align: center;
        box-shadow: 0 10px 30px -8px rgba(0, 0, 0, 0.7);
        opacity: 0;
        pointer-events: none;
        transition: opacity 0.2s ease, transform 0.2s ease;
    }

    #status-bar.show {
        opacity: 1;
        transform: translateX(-50%) translateY(0);
    }

    #status-bar.error {
        color: var(--err);
    }

    .grid-2 {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 18px;
    }

    @media (max-width: 520px) {
        .grid-2 {
            grid-template-columns: 1fr;
        }
    }
</style>
</head>
<body>
<main>
    <header>
        <div class="logo-dot"></div>
        <h1>Dying Light Spawner</h1>
        <span class="tag">Dev Tools</span>
    </header>

    <div class="card">
        <h2>Combat Modifiers</h2>
        <p class="desc">Toggle persistent gameplay states.</p>

        <label class="toggle-row">
            <span>Duplicate Items</span>
            <span class="switch">
                <input id="dupe-items" type="checkbox" onchange="setFeature('dupe-items', this.checked)">
                <span class="slider"></span>
            </span>
        </label>

        <label class="toggle-row">
            <span>Rapid Fire</span>
            <span class="switch">
                <input id="rapid-fire" type="checkbox" onchange="setFeature('rapid-fire', this.checked)">
                <span class="slider"></span>
            </span>
        </label>

        <label class="toggle-row">
            <span>Give Ammo</span>
            <span class="switch">
                <input id="give-ammo" type="checkbox" onchange="setFeature('infinite-ammo', this.checked)">
                <span class="slider"></span>
            </span>
        </label>
    </div>

    <div class="card">
        <h2>Target Player</h2>
        <p class="desc">Selected target applies to spawns, teleport and explosions.</p>

        <select id="target-player" disabled>
            <option value="">Loading players...</option>
        </select>

        <div id="player-status"><span class="pulse"></span>Waiting for the engine thread...</div>
    </div>

    <div class="grid-2">
        <div class="card">
            <h2>Spawn Item</h2>

            <label class="field-label" for="item-id">Item ID</label>
            <input id="item-id" type="number" placeholder="e.g. 1024">

            <label class="field-label" for="item-quantity">Quantity</label>
            <input id="item-quantity" type="number" value="1" min="1" max="100">

            <button onclick="spawnItem(this)">Spawn Item</button>
        </div>

        <div class="card">
            <h2>Spawn Entity</h2>

            <label class="field-label" for="entity-name">Entity Name</label>
            <input id="entity-name" maxlength="255" placeholder="Volatile_Super">

            <label class="field-label" for="entity-quantity">Quantity</label>
            <input id="entity-quantity" type="number" value="1" min="1" max="100">

            <button onclick="spawnEntity(this)">Spawn Entity</button>
        </div>
    </div>

    <div class="card">
        <h2>Actions</h2>
        <p class="desc">Do shit to the local player</p>

        <div class="row">
            <div>
                <button class="secondary" onclick="teleportToPlayer(this)">Teleport To Target (Wont work) ~ maybe host?</button>
            </div>
            <div>
                <button class="secondary" onclick="spawnExplosion(this)">Spawn Explosion</button>
            </div>
        </div>
    </div>
</main>

<div id="status-bar"></div>

<script>
const statusBar = document.getElementById("status-bar");
const targetElement = document.getElementById("target-player");
const playerStatusElement = document.getElementById("player-status");
const dupeItemsElement = document.getElementById("dupe-items");
const rapidFireElement = document.getElementById("rapid-fire");
const giveAmmoElement = document.getElementById("give-ammo");

let statusTimeout = null;

function setStatus(message, error = false) {
    statusBar.textContent = message;
    statusBar.classList.toggle("error", error);
    statusBar.classList.add("show");

    clearTimeout(statusTimeout);
    statusTimeout = setTimeout(() => statusBar.classList.remove("show"), 2600);
}

async function submit(button, path, values) {
    if (!targetElement.value) {
        setStatus("No active target player", true);
        return;
    }

    button.disabled = true;
    values.target = targetElement.value;

    try {
        const response = await fetch(path, {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: new URLSearchParams(values)
        });

        if (!response.ok) {
            const message = await response.text();
            setStatus(message || `Request failed: ${response.status}`, true);
            return;
        }

        setStatus("Command submitted");
    } catch (error) {
        setStatus(`Connection failed: ${error}`, true);
    } finally {
        button.disabled = false;
    }
}

async function refreshPlayers() {
    try {
        const response = await fetch("/api/players", { cache: "no-store" });
        if (!response.ok)
            throw new Error(`HTTP ${response.status}`);

        const data = await response.json();
        const previous = targetElement.value;
        const players = data.players || [];

        players.sort((left, right) => Number(right.local) - Number(left.local));

        const local = players.find(player => player.local);

        targetElement.replaceChildren();

        for (const player of players) {
            const option = document.createElement("option");
            option.value = player.id;
            option.textContent = player.local ? `[LOCAL] ${player.id}` : `Player ${player.index + 1} - ${player.id}`;
            targetElement.appendChild(option);
        }

        const previousStillActive = players.some(player => player.id === previous);

        targetElement.disabled = players.length === 0;
        targetElement.value = previousStillActive ? previous : local?.id || players[0]?.id || "";

        playerStatusElement.classList.toggle("live", players.length > 0);
        playerStatusElement.innerHTML = players.length
            ? `<span class="pulse"></span>${players.length} active player${players.length === 1 ? "" : "s"}`
            : `<span class="pulse"></span>No active players`;
    } catch (error) {
        targetElement.disabled = true;
        playerStatusElement.classList.remove("live");
        playerStatusElement.innerHTML = `<span class="pulse"></span>Player list unavailable: ${error}`;
    }
}

async function setFeature(name, enabled) {
    try {
        const response = await fetch("/api/feature", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: new URLSearchParams({
                name,
                enabled: enabled ? "1" : "0"
            })
        });

        if (!response.ok)
            throw new Error(await response.text() || `HTTP ${response.status}`);

        const names = {
            "dupe-items": "Duplicate Items",
            "rapid-fire": "Rapid Fire",
            "infinite-ammo": "Give Ammo"
        };

        setStatus(`${names[name] || name} ${enabled ? "enabled" : "disabled"}`);
    } catch (error) {
        setStatus(`Failed to update feature: ${error}`, true);
        refreshFeatures();
    }
}

async function refreshFeatures() {
    try {
        const response = await fetch("/api/features", { cache: "no-store" });
        if (!response.ok)
            throw new Error(`HTTP ${response.status}`);

        const features = await response.json();

        dupeItemsElement.checked = features.dupeItems;
        rapidFireElement.checked = features.rapidFire;
        giveAmmoElement.checked = features.infiniteAmmo;
    } catch (error) {
        setStatus(`Feature state unavailable: ${error}`, true);
    }
}

function spawnItem(button) {
    submit(button, "/api/spawn-item", {
        id: document.getElementById("item-id").value,
        quantity: document.getElementById("item-quantity").value
    });
}

function spawnEntity(button) {
    submit(button, "/api/spawn-entity", {
        name: document.getElementById("entity-name").value,
        quantity: document.getElementById("entity-quantity").value
    });
}

function teleportToPlayer(button) {
    submit(button, "/api/teleport-to-player", {});
}

function spawnExplosion(button) {
    submit(button, "/api/spawn-explosion", {});
}

refreshPlayers();
refreshFeatures();

setInterval(refreshPlayers, 500);
setInterval(refreshFeatures, 1000);
</script>
</body>
</html>
)HTML";

    inline int32_t ParseInteger(const httplib::Request& Request, const char* Name, int32_t Default)
    {
        if (!Request.has_param(Name))
            return Default;

        return static_cast<int32_t>(strtol(Request.get_param_value(Name).c_str(), nullptr, 10));
    }

    inline uintptr_t ParseTarget(const httplib::Request& Request)
    {
        if (!Request.has_param("target"))
            return 0;

        return static_cast<uintptr_t>(strtoull(Request.get_param_value("target").c_str(), nullptr, 0));
    }

    inline int32_t ClampQuantity(int32_t Quantity)
    {
        if (Quantity < 1)
            return 1;

        if (Quantity > 100)
            return 100;

        return Quantity;
    }

    inline void CopyString(char* Destination, const char* Source, uint32_t Capacity)
    {
        if (!Destination || !Capacity)
            return;

        uint32_t Index = 0;

        if (Source)
        {
            while (Index + 1 < Capacity && Source[Index])
            {
                Destination[Index] = Source[Index];
                ++Index;
            }
        }

        Destination[Index] = '\0';
    }

    inline void Configure()
    {
        if (Configured.exchange(true))
            return;

        // One HTTP worker keeps Commands single-producer.
        Server.new_task_queue = []
            {
                return new httplib::ThreadPool(1);
            };

        Server.Get("/", [](const httplib::Request&, httplib::Response& Response) -> void
            {
                Response.set_content(Page, "text/html; charset=UTF-8");
            });

        Server.Get("/api/features", [](const httplib::Request&, httplib::Response& Response) -> void
            {
                char Json[128]{};

                snprintf(
                    Json,
                    sizeof(Json),
                    "{\"dupeItems\":%s,\"rapidFire\":%s,\"infiniteAmmo\":%s}",
                    DupeItems.load(std::memory_order_acquire) ? "true" : "false",
                    RapidFire.load(std::memory_order_acquire) ? "true" : "false",
                    InfiniteAmmo.load(std::memory_order_acquire) ? "true" : "false"
                );

                Response.set_header("Cache-Control", "no-store");
                Response.set_content(Json, "application/json");
            });

        Server.Post("/api/feature", [](const httplib::Request& Request, httplib::Response& Response) -> void
            {
                if (!Request.has_param("name") || !Request.has_param("enabled"))
                {
                    Response.status = 400;
                    Response.set_content("Missing feature name or state", "text/plain");
                    return;
                }

                const std::string Name = Request.get_param_value("name");
                const bool Enabled = Request.get_param_value("enabled") == "1";

                if (Name == "dupe-items")
                    DupeItems.store(Enabled, std::memory_order_release);
                else if (Name == "rapid-fire")
                    RapidFire.store(Enabled, std::memory_order_release);
                else if (Name == "infinite-ammo")
                    InfiniteAmmo.store(Enabled, std::memory_order_release);
                else
                {
                    Response.status = 400;
                    Response.set_content("Unknown feature", "text/plain");
                    return;
                }

                Response.status = 204;
            });

        Server.Get("/api/players", [](const httplib::Request&, httplib::Response& Response) -> void
            {
                const PlayerList List = ReadActivePlayers();
                std::string Json = "{\"players\":[";

                for (uint32_t Index = 0; Index < List.Count; ++Index)
                {
                    char Entry[128]{};

                    snprintf(
                        Entry,
                        sizeof(Entry),
                        "%s{\"id\":\"0x%llX\",\"index\":%u,\"local\":%s}",
                        Index ? "," : "",
                        static_cast<unsigned long long>(List.Players[Index]),
                        Index,
                        List.Players[Index] == List.LocalPlayer ? "true" : "false"
                    );

                    Json += Entry;
                }

                Json += "]}";

                Response.set_header("Cache-Control", "no-store");
                Response.set_content(Json, "application/json");
            });

        Server.Post("/api/spawn-item", [](const httplib::Request& Request, httplib::Response& Response) -> void
            {
                Command Value{};
                Value.Type = CommandType::SpawnItem;
                Value.Target = ParseTarget(Request);
                Value.ID = ParseInteger(Request, "id", 0);
                Value.Quantity = ClampQuantity(ParseInteger(Request, "quantity", 1));

                if (!Value.ID)
                {
                    Response.status = 400;
                    Response.set_content("Invalid item ID", "text/plain");
                    return;
                }

                if (!IsActivePlayer(Value.Target))
                {
                    Response.status = 409;
                    Response.set_content("Target player is no longer active", "text/plain");
                    return;
                }

                if (!Commands.Push(Value))
                {
                    Response.status = 503;
                    Response.set_content("Command queue is full", "text/plain");
                    return;
                }

                Response.status = 204;
            });

        Server.Post("/api/spawn-entity", [](const httplib::Request& Request, httplib::Response& Response) -> void
            {
                if (!Request.has_param("name"))
                {
                    Response.status = 400;
                    Response.set_content("Invalid entity name", "text/plain");
                    return;
                }

                Command Value{};
                Value.Type = CommandType::SpawnEntity;
                Value.Target = ParseTarget(Request);
                Value.Quantity = ClampQuantity(ParseInteger(Request, "quantity", 1));

                CopyString(Value.Name, Request.get_param_value("name").c_str(), sizeof(Value.Name));

                if (!Value.Name[0])
                {
                    Response.status = 400;
                    Response.set_content("Invalid entity name", "text/plain");
                    return;
                }

                if (!IsActivePlayer(Value.Target))
                {
                    Response.status = 409;
                    Response.set_content("Target player is no longer active", "text/plain");
                    return;
                }

                if (!Commands.Push(Value))
                {
                    Response.status = 503;
                    Response.set_content("Command queue is full", "text/plain");
                    return;
                }

                Response.status = 204;
            });

        Server.Post("/api/teleport-to-player", [](const httplib::Request& Request, httplib::Response& Response) -> void
            {
                Command Value{};
                Value.Type = CommandType::TeleportToPlayer;
                Value.Target = ParseTarget(Request);

                if (!IsActivePlayer(Value.Target))
                {
                    Response.status = 409;
                    Response.set_content("Target player is no longer active", "text/plain");
                    return;
                }

                if (!Commands.Push(Value))
                {
                    Response.status = 503;
                    Response.set_content("Command queue is full", "text/plain");
                    return;
                }

                Response.status = 204;
            });

        Server.Post("/api/spawn-explosion", [](const httplib::Request& Request, httplib::Response& Response) -> void
            {
                Command Value{};
                Value.Type = CommandType::SpawnExplosion;
                Value.Target = ParseTarget(Request);

                if (!IsActivePlayer(Value.Target))
                {
                    Response.status = 409;
                    Response.set_content("Target player is no longer active", "text/plain");
                    return;
                }

                if (!Commands.Push(Value))
                {
                    Response.status = 503;
                    Response.set_content("Command queue is full", "text/plain");
                    return;
                }

                Response.status = 204;
            });

        Server.set_error_handler([](const httplib::Request&, httplib::Response& Response) -> void
            {
                if (Response.status == 404)
                    Response.set_content("Endpoint not found", "text/plain");
            });
    }

    static bool OpenWebPage()
    {
        SHELLEXECUTEINFOW Info{};
        Info.cbSize = sizeof(Info);
        Info.fMask = SEE_MASK_NOASYNC;
        Info.lpVerb = L"open";
        Info.lpFile = L"http://127.0.0.1:31847";
        Info.nShow = SW_SHOWNORMAL;

        if (ShellExecuteExW(&Info))
            return true;

        wchar_t Command[] = L"rundll32.exe url.dll,FileProtocolHandler http://127.0.0.1:31847";

        STARTUPINFOW Startup{};
        Startup.cb = sizeof(Startup);

        PROCESS_INFORMATION Process{};

        if (!CreateProcessW(nullptr, Command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &Startup, &Process))
        {
            printf("Failed to open web page: %lu\n", GetLastError());
            return false;
        }

        CloseHandle(Process.hThread);
        CloseHandle(Process.hProcess);
        return true;
    }

    inline bool Start()
    {
        if (ServerThread.joinable())
            return Server.is_running();

        Configure();
        ServerState.store(0, std::memory_order_release);

        ServerThread = std::thread([]
            {
                printf("[WebGUI] Starting on http://127.0.0.1:%d\n", Port);

                ServerState.store(1, std::memory_order_release);
                const bool Result = Server.listen("127.0.0.1", Port);

                printf("[WebGUI] Server stopped, result=%d\n", Result);
                ServerState.store(Result ? 0 : -1, std::memory_order_release);
            });

        for (uint32_t Attempt = 0; Attempt < 500; ++Attempt)
        {
            if (Server.is_running())
            {
                printf("[WebGUI] Server is listening\n");
                OpenWebPage();
                return true;
            }

            if (ServerState.load(std::memory_order_acquire) == -1)
                break;

            Sleep(10);
        }

        printf("[WebGUI] Failed to start server\n");

        Server.stop();

        if (ServerThread.joinable())
            ServerThread.join();

        return false;
    }

    inline void Stop()
    {
        ClearPlayers();
        Server.stop();

        if (ServerThread.joinable())
            ServerThread.join();

        ServerState.store(0, std::memory_order_release);
    }
}