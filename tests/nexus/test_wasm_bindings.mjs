import path from 'path';
import { fileURLToPath } from 'url';

async function createTopology(Nexus) {
    console.log("Creating Topology in JavaScript...");

    const leaderId = Nexus.createNode(350, 100);
    const fedId = Nexus.createNode(100, 500);
    const sedId = Nexus.createNode(350, 500);
    const medId = Nexus.createNode(600, 500);
    const router1Id = Nexus.createNode(200, 300);
    const router2Id = Nexus.createNode(500, 300);

    Nexus.formNetwork(leaderId);

    // Use the exported JoinMode enum
    Nexus.joinNetwork(fedId, leaderId, Nexus.JoinMode.AsFed);
    Nexus.joinNetwork(sedId, leaderId, Nexus.JoinMode.AsSed);
    Nexus.joinNetwork(medId, leaderId, Nexus.JoinMode.AsMed);
    Nexus.joinNetwork(router1Id, leaderId, Nexus.JoinMode.AsFtd);
    Nexus.joinNetwork(router2Id, leaderId, Nexus.JoinMode.AsFtd);

    return { leaderId, fedId, sedId, medId, router1Id, router2Id };
}

async function runTest(modulePath) {
    const absolutePath = path.resolve(modulePath);
    console.log(`Loading Nexus WASM module from: ${absolutePath}`);

    // Load the Emscripten-generated modularized ES6 module
    const { default: createNexusModule } = await import('file://' + absolutePath);

    // Initialize the module
    const Nexus = await createNexusModule();

    await createTopology(Nexus);

    console.log("Initial simulation time:", Nexus.getNow());

    console.log("Stepping Simulation...");
    let nodeStateChangedCount = 0;
    let linkUpdateCount = 0;
    let packetEventCount = 0;

    // Advance simulation and check for events
    for (let i = 0; i < 50; i++) {
        Nexus.stepSimulation(100);
        let event;
        while ((event = Nexus.pollEvent()) !== null) {
            const type = event.type;
            const data = event.data;

            if (type === "node_state_changed") {
                nodeStateChangedCount++;
                if (nodeStateChangedCount < 10) {
                    console.log(`[Event] Node ${data.id} state: ${data.role} at (${data.x}, ${data.y}) RLOC16: ${data.rloc16} ExtAddr: ${data.extAddress}`);
                }
                if (!data.extAddress) {
                    throw new Error(`Node ${data.id} state change event missing extAddress`);
                }
            } else if (type === "link_update") {
                linkUpdateCount++;
            } else if (type === "packet_event") {
                packetEventCount++;
                if (packetEventCount < 5) {
                    console.log(`[Event] Packet from ${data.srcId} to ${data.dstId} (length: ${data.length}) frame: ${data.frame.length} bytes`);
                }
                if (!data.frame || data.frame.length !== data.length) {
                    throw new Error(`Packet event from ${data.srcId} to ${data.dstId} has invalid frame data`);
                }
            }
        }
    }

    console.log(`Received ${nodeStateChangedCount} node_state_changed events`);
    console.log(`Received ${linkUpdateCount} link_update events`);
    console.log(`Received ${packetEventCount} packet_event events`);

    if (nodeStateChangedCount === 0) {
        throw new Error("Expected node_state_changed events were not received");
    }

    // Verify getNodeId
    let event;
    // We need to find an event that has extAddress to test getNodeId
    // Since we've already polled all events, we should have some node state changed events.
    // Let's assume the first few events we printed have what we need, or just use the last polled data if available.
    // Actually, let's just create a new node and test it.

    console.log("Creating a manual node...");
    const nodeId = Nexus.createNode(100, 200);
    console.log("Created node ID:", nodeId);

    // Step to trigger state change event to get its extAddress
    Nexus.stepSimulation(1);
    let foundExtAddr = null;
    while ((event = Nexus.pollEvent()) !== null) {
        if (event.type === "node_state_changed" && event.data.id === nodeId) {
            foundExtAddr = event.data.extAddress;
        }
    }

    if (foundExtAddr) {
        const resolvedId = Nexus.getNodeId(foundExtAddr);
        console.log(`Resolved ExtAddr ${foundExtAddr} to ID: ${resolvedId}`);
        if (resolvedId !== nodeId) {
            throw new Error(`getNodeId failed: expected ${nodeId}, got ${resolvedId}`);
        }
    }

    Nexus.setNodePosition(nodeId, 150, 250);
    Nexus.setNodeEnabled(nodeId, false);
    Nexus.setNodeEnabled(nodeId, true);

    console.log("Test completed successfully!");
}

const modulePath = process.argv[2];
if (!modulePath) {
    console.error("Usage: node test_wasm_bindings.mjs <path-to-nexus_live_demo.js>");
    process.exit(1);
}

runTest(modulePath).catch(err => {
    console.error("Test failed:", err);
    process.exit(1);
});
