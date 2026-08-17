# Cross-Layer Performance Analysis of IEEE 802.11b CSMA/CA and RIP in Static Wireless Ad-Hoc Networks

![NS-3](https://img.shields.io/badge/NS--3-Simulator-blue)
![C++](https://img.shields.io/badge/Language-C%2B%2B-00599C)
![802.11b](https://img.shields.io/badge/PHY%2FMAC-IEEE%20802.11b-orange)
![RIP](https://img.shields.io/badge/Routing-RIP%20(Distance--Vector)-green)
![FlowMonitor](https://img.shields.io/badge/Metrics-FlowMonitor-lightgrey)
![Topology](https://img.shields.io/badge/Topology-Ad--Hoc%20IBSS-yellow)

## 1. Overview

This repository contains an NS-3 simulation that quantifies the interaction between two independently designed OSI layers operating over the same shared wireless medium:

- **Layer 2 (MAC):** IEEE 802.11b in `AdhocWifiMac` mode, using the CSMA/CA Distributed Coordination Function (DCF) for medium access.
- **Layer 3 (Network):** RIP (Routing Information Protocol), a distance-vector routing protocol based on the distributed Bellman-Ford algorithm.

Although CSMA/CA and RIP are architecturally decoupled and designed without explicit cross-layer awareness, their operational behaviors are coupled in practice: MAC-layer queuing delay affects the timeliness of RIP update delivery, and RIP's periodic control traffic consumes channel capacity that CSMA/CA must arbitrate. This project isolates and quantifies that interaction across three controlled scenarios — **Baseline**, **Heavy Traffic**, and **Scalability** — using a single parametric C++ simulation source (`wifi_rip_project.cc`) and `ns3::FlowMonitor` for empirical measurement.

The central finding is that **performance degradation is scenario-dependent and originates from different layers**: MAC-layer contention collapse under traffic saturation versus network-layer convergence failure under increased topology diameter. Distinguishing these root causes is the primary contribution of this analysis.

---

## 2. Theoretical Background

### 2.1 CSMA/CA (DCF) Contention Management

CSMA/CA manages simultaneous transmission attempts through:

1. **Carrier Sense** — a node senses the channel before transmitting.
2. **DIFS (DCF Inter-Frame Space)** — a mandatory idle period the channel must remain free before contention begins.
3. **Random Backoff** — a countdown timer drawn uniformly from a Contention Window (CW); the timer freezes when the channel becomes busy and resumes from the same point once it clears again (freeze-and-resume), rather than restarting from zero.
4. **Exponential Backoff on Failure** — CW doubles after a collision or missing ACK, self-throttling offered load under contention.

This project uses `ns3::ConstantRateWifiManager` with a fixed physical rate (`DsssRate11Mbps` data / `DsssRate1Mbps` control), which removes dynamic rate adaptation (e.g., Minstrel) as a confounding variable. All observed throughput degradation is therefore attributable to MAC contention and channel saturation, not physical-layer rate switching.

### 2.2 CSMA/CA vs. CSMA/CD in Half-Duplex Wireless Media

CSMA/CD (wired Ethernet) assumes a node can transmit and listen simultaneously to detect a collision in progress and abort immediately. This assumption fails in RF media for two physical reasons:

| Limitation | Description |
|---|---|
| **Half-duplex radios** | A transmitting radio's own signal power is orders of magnitude greater than any received signal, masking the ability to detect a concurrent weak transmission from another node. Collision detection during transmission is effectively impossible. |
| **Hidden Terminal Problem** | Two nodes within range of a common receiver may be outside each other's radio range. Neither is aware the other is transmitting simultaneously, since neither can hear the other. This has no wired analog, where the shared medium is physically observable end-to-end. |

Consequently, IEEE 802.11 adopts **collision avoidance** (pre-emptive) rather than **collision detection** (reactive), compensating for the inability to detect collisions in-flight with explicit link-layer ACKs. A missing ACK within the expected window is interpreted as an indirect collision/error signal, triggering retransmission.

### 2.3 RIP Convergence Dynamics

RIP is a distance-vector protocol implementing a distributed Bellman-Ford computation. Each node periodically broadcasts its full routing table (hop-count distance to each known destination) to its direct neighbors. Each receiving neighbor increments the advertised distance by one hop and updates its own table only if the new path is shorter than its current known path.

Key convergence properties relevant to this study:

- **Convergence is incremental and hop-by-hop.** A topology or reachability change propagates one hop per periodic broadcast interval; it does not propagate instantaneously.
- **Convergence time scales with network diameter** `d`: a full route-informing wave requires at least `d` broadcast rounds to fully propagate.
- **Hop-count ceiling of 16 ("infinity").** RIP implicitly bounds the maximum supportable network diameter; count-to-infinity behavior after a link failure can further prolong reconvergence.
- **No parallel path computation**, unlike link-state protocols (e.g., OSPF) where each node independently computes shortest paths after a full topology flood.

These properties directly explain the blackout behavior observed in the Scalability scenario (Section 6.3).

---

## 3. Network Topology & Cross-Layer Simulation Design

| Design Element | Configuration | Rationale |
|---|---|---|
| **Topology** | Static linear/grid chain, node spacing 80–120 m | Forces genuine multi-hop RIP path discovery instead of single-hop delivery; worst-case diameter for `n` nodes is `n − 1` |
| **Mobility** | `ns3::ConstantPositionMobilityModel` | Eliminates topology dynamics as a variable; all observed effects are attributable to MAC contention or RIP convergence, not link churn |
| **MAC Mode** | `ns3::AdhocWifiMac` (IBSS) | No Access Point / Station role; all nodes act as peers, and DCF coordination is fully distributed — models a genuine ad-hoc environment |
| **Propagation Model** | `YansWifiChannelHelper::Default()` — Friis loss model | Signal attenuation follows inverse-square law with fixed propagation delay; avoids confounding effects from multipath fading or shadowing |
| **PHY/MAC Standard** | IEEE 802.11b, `ConstantRateWifiManager` (`DsssRate11Mbps` / `DsssRate1Mbps`) | Fixes the physical rate ceiling at 11 Mbps and removes rate-adaptation as a variable |
| **Routing** | `RipHelper` via `Ipv4ListRoutingHelper`, priority 10 | Sole active routing protocol; priority value is otherwise inconsequential since no competing protocol is installed |
| **Traffic Model** | `OnOffApplication` over UDP, `OnTime=1`, `OffTime=0` (i.e., CBR), 1024 B payload, port 9 | Node 0 is the fixed traffic source, transmitting concurrently to all other nodes — modeling a many-flow sink-aggregation scenario (e.g., WSN collector node) |
| **Traffic Start Offset** | Servers at t = 1 s, clients at t = 35 s | Guarantees several full RIP broadcast cycles complete before data injection begins, preventing false "No Route to Host" drops from being misattributed to MAC contention |
| **Metrics Isolation** | `FlowMonitor::InstallAll()` with strict 5-tuple filter: `sourceAddress == 10.1.1.1 && destinationPort == 9` | `InstallAll()` captures **all** IP traffic including RIP broadcast/multicast control packets; without this filter, control traffic pollutes throughput/PDR statistics for the UDP data flows under test |

**Address plan:** `10.1.1.0/24`, sequentially assigned (`Node 0 → 10.1.1.1`, `Node 1 → 10.1.1.2`, …).

---

## 4. Repository Structure

```
.
├── wifi_rip_project.cc     # Parametric NS-3 simulation (single source, all scenarios)
├── report/                 # Full technical report (PDF)
├── plots/
│   ├── baseline_throughput.png
│   ├── baseline_delay.png
│   ├── baseline_pdr.png
│   ├── heavy_throughput.png
│   ├── heavy_delay.png
│   ├── heavy_pdr.png
│   ├── scale_throughput.png
│   ├── scale_delay.png
│   └── scale_pdr.png
└── README.md
```

---

## 5. Parametric CLI Usage

All scenario variation is achieved through command-line arguments against the same compiled binary — no recompilation is required between runs.

| Argument | Type | Default | Description |
|---|---|---|---|
| `--nNodes` | `uint32_t` | `5` | Number of nodes in the linear/grid topology |
| `--dataRate` | `string` | `500kbps` | Per-flow UDP CBR data rate |
| `--scenario` | `string` | `baseline` | Free-text label: `baseline` \| `heavy` \| `scale` (used only for output tagging) |
| `--simTime` | `double` | `80.0` | Total simulation duration (seconds) |
| `--packetSize` | `uint32_t` | `1024` | UDP payload size (bytes) |
| `--gridDelta` | `double` | `120.0` | Grid/node spacing (meters) |
| `--verbose` | `bool` | `false` | Enables `LOG_LEVEL_INFO` for the `WifiRipProject` log component |

### Example invocations

```bash
# Baseline: 5 nodes, moderate load
./ns3 run "wifi_rip_project --scenario=baseline --nNodes=5 --dataRate=500kbps"

# Heavy Traffic: push the channel into saturation
./ns3 run "wifi_rip_project --scenario=heavy --nNodes=5 --dataRate=2Mbps"

# Scalability: extend network diameter under light load
./ns3 run "wifi_rip_project --scenario=scale --nNodes=15 --dataRate=500kbps --gridDelta=80"
```

---

## 6. Experimental Results & Comparative Analysis

### 6.1 Baseline Scenario (5 nodes, 500 Kbps/flow)

| Metric | Result |
|---|---|
| Throughput (all flows) | ~510 Kbps (near-nominal, including UDP/IP/MAC overhead) |
| PDR | 100% across all four flows |
| Delay pattern | Linear "staircase": ~1 ms (1-hop, Node 1) → ~5.5 ms (4-hop, Node 4) |
| Per-hop delay increment | ~1.0–1.5 ms per additional hop |

`![Baseline Throughput](plots/baseline_throughput.png)`
`![Baseline Delay](plots/baseline_delay.png)`
`![Baseline PDR](plots/baseline_pdr.png)`

Under offered load well within channel capacity, CSMA/CA arbitrates access without destructive contention. End-to-end delay grows approximately linearly with hop count, consistent with each intermediate hop contributing an independent, roughly constant increment: physical-layer reception, RIP next-hop lookup, MAC re-queuing, and re-contention for the outbound channel.

### 6.2 Heavy Traffic Scenario (5 nodes, ≥2 Mbps/flow — saturation)

| Metric | Result |
|---|---|
| Throughput (aggregate ceiling) | ~1300–1400 Kbps, regardless of higher offered load |
| PDR | Collapses below 20% for all flows |
| Delay | Sharp escalation as queues saturate |

`![Heavy Traffic Throughput](plots/heavy_throughput.png)`
`![Heavy Traffic Delay](plots/heavy_delay.png)`
`![Heavy Traffic PDR](plots/heavy_pdr.png)`

Throughput plateaus at the network's **effective capacity**, well below the 11 Mbps physical ceiling, because the aggregate must be shared across four concurrent flows, multiple relay hops, and DIFS/backoff/ACK overhead — a manifestation of the classical **multi-hop capacity degradation** effect in ad-hoc wireless networks (end-to-end capacity of an `h`-hop path scales approximately as `1/h` relative to single-hop capacity, since each relay both receives and retransmits on the same shared channel).

PDR collapse is driven by:
- **Intra-flow interference** from concurrent multi-hop relaying.
- **Hidden node exposure** in the linear topology.
- **Exponential backoff saturation** — CW growth cannot keep pace with contention volume.
- **Queue overflow** at the fixed NS-3 default queue depth.
- **MAC retry-limit exhaustion**, resulting in outright packet drops rather than delayed delivery.

This is a **channel-layer (MAC) bottleneck** — capacity is not merely reduced, it is actively wasted on failed transmission attempts that still consume airtime.

### 6.3 Scalability Scenario (15 nodes, diameter 14 hops, 500 Kbps/flow)

| Metric | Result |
|---|---|
| Nodes 1–10 | ~100% PDR, near-nominal throughput |
| Nodes 11–14 | **0% PDR, 0 Kbps throughput (total blackout)** |
| Failure mode | Abrupt cliff-edge, not gradual decay |

`![Scalability Throughput](plots/scale_throughput.png)`
`![Scalability Delay](plots/scale_delay.png)`
`![Scalability PDR](plots/scale_pdr.png)`

Under identical (light) offered load per flow, the failure mode is qualitatively different from Section 6.2. Nodes within the RIP convergence horizon reachable inside the pre-traffic 34-second window (t = 1 s to t = 35 s) receive full service. Nodes beyond that horizon (11–14) never acquire a valid route before data injection begins: packets are dropped at the source (or nearest route-less intermediate node) as **"No Route to Host"** — before ever entering MAC-layer contention. This is a **network-layer (RIP) bottleneck**, unrelated to channel saturation.

### 6.4 Cross-Scenario Comparison

| Metric | Baseline | Heavy Traffic | Scalability |
|---|---|---|---|
| Node count | 5 | 5 | 15 |
| Per-flow rate | 500 kbps | Saturating (≥2 Mbps) | 500 kbps |
| Channel state | Unsaturated | Saturated | Unsaturated (near source) |
| Throughput | Full delivery | Capped at effective capacity | Nominal for reachable nodes; zero for unreachable nodes |
| Avg. delay pattern | Staircase (hop-dependent) | Very high, unstable | Staircase for reachable nodes |
| PDR | 100% | Severe collapse (< 20%) | 100% for neighbors, 0% for distant nodes |
| Dominant failure mechanism | — | Hidden node + MAC contention collapse | RIP convergence lag |

**Interpretation:** the root cause of degradation is fundamentally different per scenario. In **Heavy Traffic**, the bottleneck is the MAC layer and physical channel capacity. In **Scalability**, the bottleneck is the network layer and the convergence speed of a distance-vector protocol. This separation is the key experimental design outcome: each scenario isolates a single independent variable while holding others constant.

---

## 7. Technical Answers to Core Research Questions

### 7.1 How does CSMA/CA manage simultaneous transmissions, and why does it outperform CSMA/CD in wireless media?

CSMA/CA (via DCF) combines carrier sensing, a mandatory DIFS idle interval, and randomized freeze-and-resume backoff drawn from a Contention Window. In the Baseline scenario, this mechanism manages four concurrent flows with zero packet loss. Under Heavy Traffic, exponential backoff prevents outright throughput collapse, holding aggregate throughput at a relatively stable ceiling — at the cost of severe PDR degradation and latency inflation.

CSMA/CD is inapplicable to wireless media because:
1. **Half-duplex radios** cannot detect a weak incoming collision signal while transmitting at much higher local power.
2. **Hidden terminals** prevent two nodes sharing a common receiver from directly sensing each other's transmissions.

802.11 therefore substitutes *avoidance* for *detection*, using explicit ACK-based confirmation to infer collisions indirectly.

### 7.2 What paths does RIP discover, and how quickly are they established?

Given the linear topology, RIP discovers the unique chain path `0 → 1 → 2 → … → k` for each destination `k` (no alternate paths exist in this topology). The distributed Bellman-Ford computation propagates hop-count vectors between direct neighbors each broadcast cycle.

- **Baseline (diameter 4):** the 34-second pre-traffic window is more than sufficient; full convergence completes in a handful of broadcast rounds, yielding 100% PDR.
- **Scalability (diameter 14):** the same 34-second window is insufficient. Nodes 1–10 converge in time; nodes 11–14 never acquire a valid route before traffic starts, producing permanent 0% PDR for those destinations.

This confirms a **non-linear relationship between network diameter and required convergence time** — a critical scalability constraint for distance-vector protocols.

### 7.3 Where is the channel saturation limit, and what happens beyond it?

The effective channel capacity in this multi-hop 802.11b topology is ~1300–1400 Kbps aggregate — significantly below the 11 Mbps physical rate, due to shared multi-hop relaying and per-frame overhead (DIFS, backoff, ACK). Beyond this point:
- Throughput plateaus (does not decrease further under increasing offered load).
- PDR collapses catastrophically, since backoff cannot resolve contention volume, queues overflow, and the MAC retry limit is exhausted for a growing fraction of packets.
- Airtime is wasted on failed transmission attempts, meaning saturation causes **actual resource loss**, not just reduced efficiency.

### 7.4 What are RIP's 16-hop limit and count-to-infinity vulnerabilities in larger-scale deployments?

| Vulnerability | Mechanism | Consequence at Scale |
|---|---|---|
| **Hop-count ceiling (16 = "infinity")** | RIP treats a hop count of 16 as unreachable | Hard upper bound on supportable network diameter; topologies with diameter ≥ 15 are fundamentally unsupportable |
| **Linear convergence scaling** | Convergence requires ≥ `d` broadcast rounds for diameter `d`, worsened by fixed periodic broadcast intervals | Larger networks require proportionally longer settling time before reliable delivery is possible — directly observed in Section 6.3 |
| **Count-to-infinity** | After a link failure, neighboring nodes can mutually and repeatedly inflate their advertised distance toward the "infinity" value before stabilizing | Reconvergence after topology change can take many broadcast cycles, especially in longer chains |

These properties make RIP structurally unsuitable for large or topologically dynamic ad-hoc networks, motivating the use of link-state protocols (e.g., OSPF) or ad-hoc-specific protocols (e.g., AODV, OLSR) designed for rapid reconvergence under mobility and scale.

**CSMA/CA at larger scale** is similarly constrained by fixed shared channel capacity: as node count or per-node concurrent flow count increases, contention grows non-linearly and per-flow fair throughput decreases; hidden-terminal exposure also increases with network size, since more node pairs share a common receiver while remaining mutually out of range. Both layer-specific constraints compound simultaneously as scale increases.

---

## 8. Build & Execution Guide

### 8.1 Prerequisites

- NS-3 (tested against a `ns3` CMake-based build; `waf`-based NS-3 versions are also compatible with equivalent syntax)
- A C++17-capable compiler toolchain
- Standard NS-3 modules: `core`, `network`, `internet`, `mobility`, `wifi`, `applications`, `flow-monitor`, `internet-apps`

### 8.2 Build Steps

```bash
# 1. Place the source file in the NS-3 scratch directory
cp wifi_rip_project.cc /path/to/ns-3-dev/scratch/

# 2. Configure (first-time setup)
cd /path/to/ns-3-dev
./ns3 configure --enable-examples

# 3. Build
./ns3 build
```

### 8.3 Running Scenarios

**Using `ns3` (CMake build system):**

```bash
./ns3 run "scratch/wifi_rip_project --scenario=baseline --nNodes=5  --dataRate=500kbps --simTime=80"
./ns3 run "scratch/wifi_rip_project --scenario=heavy    --nNodes=5  --dataRate=2Mbps   --simTime=80"
./ns3 run "scratch/wifi_rip_project --scenario=scale    --nNodes=15 --dataRate=500kbps --gridDelta=80 --simTime=80"
```

**Using `waf` (legacy build system):**

```bash
./waf --run "scratch/wifi_rip_project --scenario=baseline --nNodes=5 --dataRate=500kbps --simTime=80"
```

### 8.4 Output

Each run prints per-flow statistics to `stdout`, filtered to the UDP data flows originating at `10.1.1.1`:

```
--- Simulation Results (baseline, 5 nodes, 500kbps) ---
Flow <id> (10.1.1.1 -> 10.1.1.X)
  Tx Packets: ...
  Rx Packets: ...
  PDR:        ... %
  Throughput: ... Kbps
  Avg Delay:  ... ms
```

Redirect output to a log file and post-process externally (e.g., with a Python/pandas script or the NS-3 XML `FlowMonitor` trace) to regenerate the throughput, delay, and PDR bar charts referenced in Section 6.
