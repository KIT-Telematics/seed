# TODO

## Addis

- [x] implement: campus network
- [x] examine: omnetpp pcap behavior outside of docker  
      *same segfault as within docker*
- [x] implement: pbce in omnetpp (if pcap still segfaults)  
      *dropped: only support for output action but set field actions needed*
      *(actions defined as array of ofp_action_output in*
      *openflow/messages/OFP_Flow_Mod.msg:46)*
- [ ] implement: metrics
    - [x] TxBytes
    - [x] RxBytes
    - [x] FCT
    - [ ] Flow-Throughput

## Michael

- [X] fix: packet routing
- [X] fix: link-change
- [X] fix: error-rate
- [ ] implement: process event
- [ ] implement: host selection
- [X] implement: location hints
- [X] implement: animation desc
- [ ] implement: metrics
- [ ] implement: ISP/IXP network

## Felix

- [X] fix: port collisions iperf
- [ ] fix: link-down with reup
- [ ] implement: fire process
- [ ] implement: metrics
- [ ] implement: Simple DC Traffic showing effects
- [ ] implement: Complex DC Traffic
- [ ] implement: iTAP



# To Be Seen

- [ ] Flow Table Usage Plot (PBCE + campus + omnetpp)
- [ ] Animation, Plain Stats (ECMP + prototype + ns-3)
- [ ] Packet Dump with IPs (iTAP + DC + mininet)
