# RAGEInsight

**RAGEInsight** is a native C++ runtime inspection and AI telemetry toolkit for **Red Dead Redemption 2**, built around **ScriptHookRDR2 V2**.

The goal is to provide a developer-oriented view into RDR2's runtime state: inspect entities, observe NPC behavior, trace AI state changes, investigate line-of-sight and navigation behavior, and interact with the game through an external real-time developer console.

This is not intended to be a traditional trainer. RAGEInsight is primarily an **engineering, debugging, and reverse-engineering project** focused on understanding how RDR2 and the RAGE runtime behave while the game is running.

---

## Project Goals

RAGEInsight is being designed around a simple idea:

> What if we could watch RDR2's NPCs and game systems behave in real time from a developer console?

The project aims to expose useful runtime information such as:

* Entity handles and model hashes
* Player and NPC world positions
* Heading, velocity, health, and movement state
* Nearby entities
* Camera-targeted entity inspection
* Line-of-sight state
* Raycast results
* Combat and flee state
* NPC task and behavior changes
* Relationship and interaction state
* AI telemetry over time
* Runtime traces that can be recorded and compared

Longer-term, the project may investigate lower-level RAGE structures such as task managers, navigation state, perception systems, entity pools, and other internal runtime structures where practical.

---

## Concept

RAGEInsight consists of two primary runtime components and a shared protocol library.

```text
                         RDR2.exe
                            │
                            ▼
                    ScriptHookRDR2 V2
                            │
                            ▼
                  ┌───────────────────┐
                  │  RAGEInsight.asi  │
                  │                   │
                  │   Native C++      │
                  │                   │
                  │ Entity Inspection │
                  │ AI Telemetry      │
                  │ Native Calls      │
                  │ Runtime Tracing   │
                  └─────────┬─────────┘
                            │
                            │ Named Pipe / IPC
                            │
                            ▼
                  ┌───────────────────┐
                  │ RAGEInsight.Console
                  │                   │
                  │   Native C++      │
                  │                   │
                  │ Command Shell     │
                  │ Telemetry Output  │
                  │ Trace Viewer      │
                  └───────────────────┘
```

The in-game component runs inside the RDR2 process and communicates with a separate developer console using IPC.

This keeps game-facing code isolated from presentation, command parsing, logging, and future tooling.

---

# Architecture

## RAGEInsight.Hook

`RAGEInsight.Hook` is a native x64 C++ DLL built as an `.asi` plugin.

It is loaded into the RDR2 process and integrates with ScriptHookRDR2 V2.

Responsibilities include:

* ScriptHook initialization
* RDR2 native invocation
* Entity inspection
* NPC inspection
* Camera raycasting
* Telemetry collection
* Command execution
* AI state monitoring
* Communication with the external console

The hook is intentionally kept separate from the console application.

RDR2 native calls are executed from the ScriptHook game/script thread rather than directly from IPC worker threads.

---

## RAGEInsight.Console

`RAGEInsight.Console` is a standalone native C++ application.

It communicates with the in-game hook and provides an interactive development shell.

Example:

```text
RAGE Insight

Connecting to RDR2...
Connected.

> target

PED SELECTED
────────────────────────────────────
Handle:       0x18A4
Model:        A_M_M_VALTOWNFOLK_01
Distance:     4.21 m
Health:       200
Heading:      173.8°

> watch ai

[23:12:04.022] LOS acquired → PLAYER
[23:12:05.817] Ped turned toward PLAYER
[23:12:07.192] Ped started moving
[23:12:09.014] Combat state: false → true
[23:12:09.016] Combat target: NONE → PLAYER
```

Planned commands include:

```text
help
ping
player
target
inspect
nearby
position
health
los
raycast
watch
watch ai
watch combat
trace ai
trace stop
clear
quit
```

---

## RAGEInsight.Protocol

`RAGEInsight.Protocol` is a shared static C++ library used by both the hook and console.

It defines the IPC contract between the two applications.

This includes:

* Commands
* Message types
* Entity snapshots
* NPC snapshots
* Telemetry events
* Serialization
* Protocol versioning

The protocol project intentionally has no dependency on ScriptHook or RDR2-specific headers.

---

# Repository Structure

```text
RAGEInsight/
│
├── README.md
├── LICENSE
├── .gitignore
│
├── RAGEInsight.sln
│
├── docs/
│   ├── architecture.md
│   ├── protocol.md
│   ├── commands.md
│   └── rdr2-research.md
│
├── external/
│   └── ScriptHookRDR2/
│       ├── include/
│       └── lib/
│
├── src/
│   │
│   ├── RAGEInsight.Hook/
│   │   ├── dllmain.cpp
│   │   ├── ScriptMain.cpp
│   │   ├── ScriptMain.hpp
│   │   │
│   │   ├── Application/
│   │   ├── Commands/
│   │   ├── Game/
│   │   ├── Inspection/
│   │   ├── IPC/
│   │   └── Telemetry/
│   │
│   ├── RAGEInsight.Console/
│   │   ├── main.cpp
│   │   │
│   │   ├── Application/
│   │   ├── Commands/
│   │   ├── Display/
│   │   ├── IPC/
│   │   └── Telemetry/
│   │
│   └── RAGEInsight.Protocol/
│       ├── Commands/
│       ├── Messages/
│       └── Serialization/
│
├── tests/
│
└── tools/
```

---

# IPC

Initial communication between the game plugin and developer console uses a Windows named pipe:

```text
\\.\pipe\RAGEInsight
```

The intended execution model is:

```text
Console
   │
   │ command
   ▼
Named Pipe
   │
   ▼
IPC Worker Thread
   │
   ▼
Thread-Safe Command Queue
   │
   ▼
ScriptHook Game Thread
   │
   ▼
RDR2 Native Call
   │
   ▼
Telemetry / Result
   │
   ▼
Named Pipe
   │
   ▼
Console
```

Game-native functions should never be invoked directly from arbitrary IPC worker threads.

This boundary is deliberate and is intended to keep interaction with the RDR2 runtime predictable and safe.

---

# AI Inspection

One of the primary goals of RAGEInsight is observing NPC behavior.

Rather than pretending to expose literal NPC "thoughts," RAGEInsight attempts to inspect measurable runtime state and present it in a human-readable form.

For example:

```text
NPC STATE
────────────────────────────────────

Entity
  Handle:        0xD91
  Model:         A_M_M_RANCHER_01

Perception
  Player LOS:    YES
  Distance:      4.8 m
  Facing Player: YES

Behavior
  In Combat:     NO
  Fleeing:       NO
  Ragdoll:       NO

Movement
  Speed:         0.0 m/s
  Heading:       79.3°

Observed Change
  NPC turned to face PLAYER.
```

A future interpretation layer may summarize observed state:

```text
Observed:
  - NPC has line of sight to Arthur
  - NPC turned toward Arthur
  - NPC is not fleeing
  - NPC is not in combat

Interpretation:
  The NPC appears to have noticed Arthur and is actively
  observing him, but has not entered a hostile state.
```

These interpretations are derived from observed runtime state and should not be considered literal internal dialogue.

---

# AI Tracing

RAGEInsight is intended to support event-based AI tracing.

Instead of dumping every value every frame, snapshots can be compared and meaningful state transitions reported.

Example:

```text
> trace ai

AI TRACE ENABLED
Ped: 0xD91

[00000.000] Initial state captured

[00001.338] LOS acquired → PLAYER

[00001.920] Ped turned toward PLAYER

[00004.981] PLAYER drew weapon

[00004.983] Combat:
             false → true

[00004.984] Combat target:
             NONE → PLAYER

[00005.015] Weapon:
             NONE → CATTLEMAN_REVOLVER
```

Eventually, traces may be exported for offline comparison and analysis.

---

# Planned Development

### Phase 1 — Runtime Connection

* Load `RAGEInsight.asi` through ScriptHookRDR2 V2
* Create named-pipe IPC server
* Connect external console
* Implement `ping` / `pong`
* Establish clean shutdown behavior

### Phase 2 — Player Inspection

* Player entity handle
* Coordinates
* Heading
* Velocity
* Health
* Current mount
* Basic game-state telemetry

### Phase 3 — Entity Targeting

* Camera raycasting
* Select entity under crosshair/camera
* Entity type detection
* Model hash inspection
* Distance and coordinates
* World-space hit information

### Phase 4 — NPC Inspector

* Ped state
* Health
* Movement
* Combat state
* Flee state
* Relationship state
* Weapon state
* LOS to player

### Phase 5 — AI Telemetry

* Snapshot comparison
* Event-based state changes
* Behavior monitoring
* AI trace recording
* Runtime trace export

### Phase 6 — Deeper Runtime Research

Potential areas of investigation:

* Ped task trees
* Task manager structures
* Navigation state
* Perception state
* Entity pools
* Scenario systems
* RAGE runtime structures
* Internal AI behavior data

These features depend heavily on what can be safely and reliably discovered through runtime research.

---

# Requirements

Current development targets:

* Windows 10 / 11
* x64
* Visual Studio 2022 or newer
* MSVC
* C++20 / C++23
* Red Dead Redemption 2
* ScriptHookRDR2 V2

---

# Building

Clone the repository and open:

```text
RAGEInsight.sln
```

Build using:

```text
Configuration: Debug or Release
Platform:      x64
```

The hook project should produce:

```text
RAGEInsight.asi
```

The console project should produce:

```text
RAGEInsight.Console.exe
```

The `.asi` plugin is placed in the Red Dead Redemption 2 installation directory alongside the required ScriptHookRDR2 V2 files.

---

# Development Philosophy

This project intentionally aims to follow normal software-engineering practices despite operating in a modding and reverse-engineering environment.

That means:

* Small, focused components
* Clear separation of responsibilities
* Minimal work inside `DllMain`
* Explicit thread ownership
* No RDR2 native calls from arbitrary worker threads
* Shared strongly typed protocol definitions
* RAII for native Windows resources
* Modern C++ ownership semantics
* High compiler warning levels
* Avoidance of unnecessary global state
* Documentation of discovered runtime behavior
* Reproducible experiments where possible

The goal is not simply to make something that works.

The goal is to understand **why it works**.

---

# Status

RAGEInsight is currently under active early development.

Initial milestones focus on establishing the ScriptHookRDR2 V2 plugin, IPC bridge, external console, and basic entity inspection before moving into deeper AI telemetry and runtime analysis.

Expect interfaces and internal architecture to change while the project is being developed.

---

# Disclaimer

RAGEInsight is an independent personal project and is not affiliated with, endorsed by, or supported by Rockstar Games or Take-Two Interactive.

Red Dead Redemption, Red Dead Redemption 2, Rockstar Games, RAGE, and related names and trademarks belong to their respective owners.

This project is intended for educational, debugging, research, and single-player modding purposes.
