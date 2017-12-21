from pox.openflow.discovery import Discovery
from pox.core import core
from pox.core import EventMixin
from pox.lib.addresses import IPAddr, IPAddr6, EthAddr
import pox.openflow.libopenflow_01 as of
import pox.lib.packet as pkt
import random
import pox.lib.packet.ethernet as ethernet
import pox.lib.packet.arp as arp
from pox.openflow import PacketIn
import sys
import time


FLOW_IDLE_TIMEOUT = 100
ARP_DROP_DURATION = 100

log = core.getLogger()



class iTAP (EventMixin):
    _eventMixin_events = set([
        PacketIn
    ])

    def __init__(self, timeout):
        def start ():
            core.openflow.addListeners(self)
            core.openflow_discovery.addListeners(self)
            log.debug("Listeners added")

        FLOW_IDLE_TIMEOUT = timeout
        self.ip_alias_map={}
        self.arp_blacklist={}
        core.call_when_ready(start, ("openflow", "openflow_discovery", "topo", "host"))
        log.debug("Callback registered")


    def _handle_LinkEvent(self, event):
        return #TODO

    def _handle_PacketIn(self, event):
        self._handle_openflow_PacketIn(event)


    def _handle_openflow_PacketIn(self, event):

        packet = event.parsed
        log.debug("Packet in from %s on port %s, its type is %s", event.dpid, event.port, pkt.ETHERNET.ethernet.getNameForType(packet.type))

        if packet.type not in [packet.ARP_TYPE, packet.IP_TYPE]:
            log.debug("Packet not ARP or IP - ignoring")
            return


        # if arp, flood arp if IP unknown, answer if known
        a = packet.find('arp')
        if a:
            # Check if destination is known then install flow and forward

            # if core.host.host_map.get(packet.next.protodst) and packet.next.opcode is arp.arp.REQUEST:
            #
            #     r = arp()
            #     r.protodst = packet.next.srcip
            #     r.protosrc = packet.next.dstip
            #     r.opcode = r.REPLY
            #
            #     e = ethernet(type=ethernet.ARP_TYPE, src=ethernet.ETHER_BROADCAST,
            #                  dst=packet.src)
            #     e.set_payload(r)
            #     msg = of.ofp_packet_out()
            #     msg.data = e.pack()
            #     (dpid, port) = core.host.host_map[packet.next.protodst]
            #     msg.actions.append(of.ofp_action_output(port=port))
            #     msg.in_port = event.port
            #     core.openflow.getConnection(dpid).send(msg)
            #
            #     msg = of.ofp_flow_mod()
            #     msg.


            # Check if exists and install flow if necessary
            log.debug("Received Arp packet form %s on dpid %s, searching for %s", packet.next.protosrc, event.dpid, packet.next.protodst)

            if self.arp_blacklist.get((packet.next.protodst, packet.next.protosrc)) and self.arp_blacklist[(packet.next.protodst, packet.next.protosrc)] > time.time():
                log.debug("Received ARP from %s to %s within timout - dropping", packet.next.protosrc, packet.next.protodst)
                return
            source = packet.next.protosrc
            destination = packet.next.protodst

            #  TODO: relay arps directly since they can not be altered: Requires match and rule creation only on IP
            # packet and arp answer directly with tracking of already answered
            dst = core.host.host_map.get(destination)
            if dst:
                log.debug("%s is known - its on %s, creating rule", destination, dst[0])
                # create direct rule
                if dst[0] == event.connection.dpid:
                    log.debug("%s and %s are on the same switch",source, destination)
                    msg = of.ofp_flow_mod()
                    #msg.match = of.ofp_match()
                    msg.match.dl_type = 0x800  # Match IP packets
                    msg.match.nw_src = source
                    msg.match.nw_dst = destination
                    msg.actions.append(of.ofp_action_output(port=dst[1]))
                    event.connection.send(msg)
                    msg.match.dl_type = 0x806  # Match ARP packets
                    event.connection.send(msg)

                    msg.match = msg.match.flip()
                    msg.actions = []
                    msg.actions.append(of.ofp_action_output(port=event.port))
                    event.connection.send(msg)
                    msg.match.dl_type = 0x800  # Match IP packets
                    event.connection.send(msg)
                    return

                else: # Path with more than one hop
                    path = core.topo.query(event.port, event.connection.dpid, dst[0], dst[1])
                    log.debug("The Path is:==========\n" + str(path) + "\n=======")
                    if not path:
                        log.error("ERROR: Path from " + str(event.connection.dpid) + " to " + str(dst[0]) + " could not be found!")
                        return
                    # create pairing
                    if not (destination, source) in self.ip_alias_map:
                        newIP1 = IPAddr(int(random.randint(0, 32)))
                        newIP2 = IPAddr(int(random.randint(0, 32)))
                        self.ip_alias_map[(destination, source)] = (newIP1, newIP2)
                        self.ip_alias_map[(source, destination)] = (newIP2, newIP1)
                    # network entry point, dir forward
                    msg = of.ofp_flow_mod()
                    msg.match = of.ofp_match()
                    msg.match.nw_src = source
                    msg.match.nw_dst = destination
                    msg.actions.append(of.ofp_action_dl_addr.set_src(EthAddr("00:00:00:00:00:00")))
                    msg.actions.append(of.ofp_action_dl_addr.set_dst(EthAddr("00:00:00:00:00:00")))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_src(IPAddr(self.ip_alias_map[(destination, source)][0])))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_dst(IPAddr(self.ip_alias_map[(destination, source)][1])))
                    msg.actions.append(of.ofp_action_output(port=path[0][2]))
                    msg.match.dl_type = 0x806  # Match ARP packets
                    print(msg)
                    event.connection.send(msg)
                    msg.match.dl_type = 0x800  # Match IP packets
                    event.connection.send(msg)

                    # network entry point, dir backward
                    msg = of.ofp_flow_mod()
                    msg.match = of.ofp_match()
                    msg.match.nw_src = self.ip_alias_map[(destination, source)][1]
                    msg.match.nw_dst = self.ip_alias_map[(destination, source)][0]
                    msg.actions.append(of.ofp_action_dl_addr.set_src(
                        core.host.host_map[destination][2]))
                    msg.actions.append(of.ofp_action_dl_addr.set_dst(
                        core.host.host_map[source][2]))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_src(destination))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_dst(source))
                    msg.actions.append(of.ofp_action_output(port=event.port))
                    msg.match.dl_type = 0x806  # Match ARP packets

                    event.connection.send(msg)
                    msg.match.dl_type = 0x800  # Match IP packets
                    event.connection.send(msg)
                    print(str(msg))

                    # network exit point, dir forward

                    msg = of.ofp_flow_mod()
                    msg.match = of.ofp_match()
                    msg.match.nw_src = self.ip_alias_map[(destination, source)][0]
                    msg.match.nw_dst = self.ip_alias_map[(destination, source)][1]
                    msg.actions.append(of.ofp_action_dl_addr.set_src(
                        core.host.host_map[source][2]))
                    msg.actions.append(of.ofp_action_dl_addr.set_dst(
                        core.host.host_map[destination][2]))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_src(source))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_dst(destination))
                    msg.actions.append(of.ofp_action_output(port=core.host.host_map[destination][1]))

                    msg.match.dl_type = 0x806  # Match ARP packets
                    print(str(msg))
                    core.openflow.getConnection(path[-1][1]).send(msg)
                    msg.match.dl_type = 0x800  # Match IP packets
                    core.openflow.getConnection(path[-1][1]).send(msg)

                    # network exit point, dir backward
                    msg = of.ofp_flow_mod()
                    msg.match = of.ofp_match()
                    msg.match.nw_src = source
                    msg.match.nw_dst = destination
                    msg.actions.append(of.ofp_action_dl_addr.set_src(EthAddr("00:00:00:00:00:00")))
                    msg.actions.append(of.ofp_action_dl_addr.set_dst(EthAddr("00:00:00:00:00:00")))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_src(
                            IPAddr(self.ip_alias_map[(destination, source)][1])))
                    msg.actions.append(
                        of.ofp_action_nw_addr.set_dst(
                            IPAddr(self.ip_alias_map[(destination, source)][0])))
                    msg.actions.append(of.ofp_action_output(port=path[-1][2]))
                    #print(path[-1][1])
                    msg.match.dl_type = 0x806  # Match ARP packets
                    core.openflow.getConnection(path[-1][1]).send(msg)
                    msg.match.dl_type = 0x800  # Match IP packets
                    core.openflow.getConnection(path[-1][1]).send(msg)


                    # create rules in between
                    for hop in path[1:-1]:
                        msg = of.ofp_flow_mod()
                        msg.match = of.ofp_match()
                        msg.match.nw_src = self.ip_alias_map[(destination, source)][0]
                        msg.match.nw_dst = self.ip_alias_map[(destination, source)][1]
                        msg.actions.append(of.ofp_action_output(port=hop[2]))
                        msg.match.dl_type = 0x806  # Match ARP packets
                        print(msg)
                        core.openflow.getConnection(hop[1]).send(msg)
                        msg.match.dl_type = 0x800  # Match IP packets
                        core.openflow.getConnection(hop[1]).send(msg)

                        msg.match = msg.match.flip()
                        msg.actions.append(of.ofp_action_output(port=hop[0]))
                        msg.match.dl_type = 0x806  # Match ARP packets
                        core.openflow.getConnection(hop[1]).send(msg)
                        msg.match.dl_type = 0x800  # Match IP packets
                        core.openflow.getConnection(hop[1]).send(msg)

                # deliver packet
                msg = of.ofp_packet_out()
                msg.data = packet.pack()
                msg.actions.append(of.ofp_action_output(port=dst[1]))



            # flood
            elif packet.next.opcode is packet.next.REQUEST:
                log.debug("Target not known, flooding")
                for conn in core.openflow.connections:
                    counter = 0
                    msg = of.ofp_packet_out()
                    msg.data = packet.pack()
                    for port in conn.ports:
                        if core.openflow_discovery.is_edge_port(conn.dpid, port):
                            msg.actions.append(of.ofp_action_output(port=port))
                            counter += 1
                    if counter > 0:
                         conn.send(msg)
                self.arp_blacklist[(destination, source)] = time.time() + ARP_DROP_DURATION

            # No arp tracking in this case necessary since arp answers create rule and are forwarded to destination
            # TODO: also handle IP packets (rule timeout)
            # TODO: Track and create rule timeouts

#     def _handle_packetIn2 (self, event):
#
#         packet = event.parsed
#         if packet.type not in [ethernet.ethernet.IP_TYPE, ethernet.ethernet.ARP_TYPE]:
#             return
#
#         # create entry in map
#         # create separate rule for arp and ip
#         # flood arp packet (with src ip = broadcast and create unpack rule if src ip != broadcast
#         # create rule for ip traffic in both directions
#         # create forward rule for arp
#         if packet.type is ethernet.ethernet.ARP_TYPE:
#
#             # create mapping if nonexistent
#             if not (packet.next.protodst, packet.next.protosrc) in ip_alias_map:
#                 newIP1 = IPAddr(random.randint)
#                 newIP2 = IPAddr(int(random.randint))
#                 ip_alias_map[(packet.next.protodst, packet.next.protosrc)] = (newIP1, newIP2)
#                 ip_alias_map[(packet.next.protosrc, packet.next.protodst)] = (newIP2, newIP1)
#
#
#
#             # create adjacency entry for revrese if nonexistent
#             if not packet.next.protosrc in self.adjacency:
#                 # argument 2 = True if Host is directly connected
#                 self.adjacency[packet.next.protosrc] = (event.port, packet.src is packet.next.hwsrc)
#
#
#             # create reverse rules if path to dst is known (path is not needed anymore)
#             if packet.next.protodst in self.adjacency:
#                 # create rule ip reverse and arp reverse exact match reverse
#                 if packet.src is packet.next.hwsrc:
#                     # if ... create unpacking rule
#                     #self.ip_map[packet.next.protosrc] = (event.port, True)
#
#                     self.ip_map[(packet.next.protodst, packet.next.protosrc)] = (event.port, True)
#
#
#                 else:
#                     # create only conserving rule - no mod
#                     #self.ip_map[packet.next.protosrc] = (event.port, False)
#
#
#                 # send back arp over known port (src = ANY
#
#
#
#
#                 packet.src = ethernet.ETHER_ANY   # FIXME
#                 msg = of.ofp_packet_out()
#                 msg.data = packet.pack()
#                 msg.actions.append(of.ofp_action_output(port=))
#                 msg.in_port = event.port
#                 event.connection.send(msg)
#
#                 #
#                 # r = arp()
#                 # r.protodst = packet.next.protodst
#                 # r.opcode =
#                 #
#                 # e = ethernet(type=ethernet.ARP_TYPE, src=ethernet.ETHER_ANY,
#                 #              dst=ethernet.ETHER_ANY)
#                 # e.set_payload(r)
#                 # msg = of.ofp_packet_out()
#                 # msg.data = e.pack()
#                 # msg.actions.append(of.ofp_action_output(port=event.port))
#                 # msg.in_port = event.port
#                 # event.connection.send(msg)
#
#             else:
#
#                 # flood arp packet with mac src = ANY to show it is relayed
#
#
#
#
#
#
#
#
#     def _handle_PacketIn3 (self, event):
#
#         packet = event.parsed
#
#         if packet.type not in [ethernet.ethernet.IP_TYPE, ethernet.ethernet.ARP_TYPE]:
#             return
#         if packet.next.srcip not in self.ip_map and packet.next.srcip is not IP_ANY:
#             self.ip_map[packet.next.srcip] = (event.connection.dpid, event.port)
#         else:
#             assert self.ip_map[packet.next.srcip] == (event.connection.dpid, event.port)
#
#         # answer arp if target is known
#         #if packet.type == ethernet.ethernet.ARP_TYPE:
#         if packet.type is ethernet.ethernet.ARP_TYPE:
#             if packet.next.opcode is packet.next.REQUEST:
#                 # answer if ip is known and request is not relayed (it should then be answered by a host)
#                 if packet.next.dstip in self.ip_map and packet.next.srcip is not IP_ANY:
#                     r = arp()
#                     r.protodst = packet.next.srcip
#                     r.opcode = r.REPLY
#
#                     e = ethernet(type=ethernet.ARP_TYPE, src=ethernet.ETHER_BROADCAST,
#                                  dst=ethernet.ETHER_BROADCAST)
#                     e.set_payload(r)
#                     msg = of.ofp_packet_out()
#                     msg.data = e.pack()
#                     msg.actions.append(of.ofp_action_output(port=event.port))
#                     msg.in_port = event.port
#                     event.connection.send(msg)
#                 # flood if dst is not known
#                 elif packet.next.dstip not in self.ip_map:
#                     r = arp()
#                     r.protodst = packet.next.dstip
#                     r.opcode = r.REQUEST
#
#                     #for port in [x for x in self.ports and not packet.port]:
#                     e = ethernet(type=ethernet.ARP_TYPE, src=ethernet.ETHER_BROADCAST,
#                                      dst=ethernet.ETHER_BROADCAST)
#                     e.set_payload(r)
#                     msg = of.ofp_packet_out()
#                     msg.data = e.pack()
#                     msg.actions.append(of.ofp_action_output(port=of.OFPP_FLOOD))
#                     msg.in_port = event.port
#                     event.connection.send(msg)
#                     return
#                 # reply turn if dst is known by now
#                 else:
#
#             actions.append(of.ofp_action_nw_addr)
#
#             match = of.ofp_match()
#
#             msg = of.ofp_flow_mod(command=of.OFPFC_ADD, idle_timeout=FLOW_IDLE_TIMEOUT,
#                                   hard_timeout=of.OFP_FLOW_PERMANENT, buffer_id=event.ofp.buffer_id,
#                                   actions=actions, match=match)
#
#         # is ip or arp -> update IP map
#         # is arp -> flood
#         # is ipv4, create path - both ways
#         # deliver packet to dest
#         #
#
#
#         #ip_packet = packet.find('ipv4')
#         if ip_packet is None: return
#         neIP1 = IPAddr(random.randint)
#         neIP2 = IPAddr(int(random.randint))
#
#         tuple = ip_packet.srcip.toUnsigned()
#
#
#     def install_path(self, src, dst):
#
#         def get_path():
#             return
#
#         if path[0] is not self:
#             return
#
#         if src == dst:
#
#         msg = of.ofp_flow_mod()
#         msg.match = of.ofp_match()
#         msg.match.nw_src = src
#         msg.match.get_nw_dst(dst)
#         msg = of.ofp_flow_mod(command=of.OFPFC_ADD, idle_timeout=FLOW_IDLE_TIMEOUT,
#                               hard_timeout=of.OFP_FLOW_PERMANENT, buffer_id=event.ofp.buffer_id,
#                               actions=actions, match=match)
#
#
#
#
# class Host():
#     def __init__(self, event):
#         packet = event.parsed
#
#         self.mac = ""
#         self.ip = ""

def launch(timeout=100):
    core.registerNew(iTAP, timeout=timeout)