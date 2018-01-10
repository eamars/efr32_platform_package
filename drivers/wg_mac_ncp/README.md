# WG_MAC Wireless Protocol -- NCP implementation

`wg_mac_ncp` module contains the implementation of simple MAC protocol as a data sink, which relays
sensor data from Hatch to cloud. 

Following sections briefly describes how protocol is designed and implemented. Please refer to source
code for more specific information.

Transceiver States
-------------
Unlike Hatch, the network co-processor have access to wall power. The transceiver only toggles between two
states, either transmit or receive. 

![transceiver_state_ncp](../../resources/transceiver_state_ncp.png)

The network co-processor will enter receive state on startup, or right after the successful transmission. 
The receive state can be interrupted by either user and the network stack. The timing window can be found below:

![receive_window](../../resources/receive_window.png)

Bookkeeping
-----------
The network co-processor maintains a list of clients information. It handles the join request from new client/repeated
client. To reduce the overhead and avoid possible security issue, the network co-processor will generate a locally unique
id for newly joined devices as identities. Following diagram shows how clients are handled when `JoinRequest` packet is
received. 

![ncp_accepts_clients](../../resources/ncp_accepts_clients.png)

The bookkeeping thread is executed periodically to validate the state of clients. For any client that failed to 
send acknowledgement, the network co-processor will attempt to retransmit. The client will be removed from list
if either no response or haven't seen for a long time (those thresholds are configurable). 

![ncp_bookkeeping](../../resources/ncp_bookkeeping.png)

Downlink Channel
----------------
Due to the fact that clients only open receive window for a certain mount of time, downlink message need to be queued 
and waiting for uplink message from that specific client. 
