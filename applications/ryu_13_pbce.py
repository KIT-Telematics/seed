import networkx as nx
import os
from ryu import cfg
from ryu.app.ofctl import api
from ryu.base import app_manager
from ryu.controller import dpset, ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.lib import hub
from ryu.lib.packet import arp
from ryu.lib.packet import ethernet
from ryu.lib.packet import ether_types
from ryu.lib.packet import ipv4
from ryu.lib.packet import in_proto
from ryu.lib.packet import packet
from ryu.lib.packet import tcp
from ryu.lib.packet import udp
from ryu.ofproto import ofproto_v1_3
from ryu.topology import event, switches
import time

def dscp_in(dscp):
    port = dscp
    assert 0 < port and port < 32
    return port

def in_dscp(port):
    assert 0 < port and port < 32
    return port

def dscp_out(dscp):
    port = dscp - 32
    assert 0 < port and port < 32
    return port

def out_dscp(port):
    assert 0 < port and port < 32
    return port + 32

def replace_fields(ofp_parser, msg, **kwargs):
    msg.match = ofp_parser.OFPMatch(**dict(msg.match.items(), **kwargs))

def rewrite_msg(datapath, msg, export):
    ofp = datapath.ofproto
    ofp_parser = datapath.ofproto_parser
    assert(isinstance(msg, ofp_parser.OFPFlowMod) or
           isinstance(msg, ofp_parser.OFPPacketOut))

    if isinstance(msg, ofp_parser.OFPFlowMod):
        assert msg.match['in_port'] is not None
        replace_fields(ofp_parser, msg,
                       ip_dscp=in_dscp(msg.match['in_port']), in_port=export)
        assert(
            len(msg.instructions) == 1 and
            isinstance(msg.instructions[0],
                ofp_parser.OFPInstructionActions) and
            msg.instructions[0].type == ofp.OFPIT_APPLY_ACTIONS
        )
        actions = msg.instructions[0].actions
    if isinstance(msg, ofp_parser.OFPPacketOut):
        msg.in_port = export
        actions = msg.actions

    assert (len(actions) == 1 and
            isinstance(actions[0], ofp_parser.OFPActionOutput))
    out_action = actions[0]
    actions.insert(0, ofp_parser.OFPActionSetField(
        ip_dscp=out_dscp(out_action.port)))
    out_action.port = ofp.OFPP_IN_PORT

def eviction_rule(datapath, command, evport, dlport):
    ofp = datapath.ofproto
    ofp_parser = datapath.ofproto_parser
    return ofp_parser.OFPFlowMod(
        datapath,
        command=command,
        out_port=ofp.OFPP_ANY,
        out_group=ofp.OFPG_ANY,
        match=ofp_parser.OFPMatch(
            in_port=evport,
            eth_type=ether_types.ETH_TYPE_IP,
            ip_dscp=0
        ),
        instructions=[
            ofp_parser.OFPInstructionActions(
                ofp.OFPIT_APPLY_ACTIONS,
                actions=[
                    ofp_parser.OFPActionSetField(ip_dscp=in_dscp(evport)),
                    ofp_parser.OFPActionOutput(
                        port=ofp.OFPP_IN_PORT if evport == dlport else dlport
                    )
                ]
            )
        ],
        priority=ofp.OFP_DEFAULT_PRIORITY-1
    )

def backflow_rules(datapath, command, outport):
    ofp = datapath.ofproto
    ofp_parser = datapath.ofproto_parser
    return [ofp_parser.OFPFlowMod(
        datapath,
        command=command,
        out_port=ofp.OFPP_ANY,
        out_group=ofp.OFPG_ANY,
        match=ofp_parser.OFPMatch(
            in_port=outport,
            eth_type=ether_types.ETH_TYPE_IP,
            ip_dscp=out_dscp(outport)
        ),
        instructions=[
            ofp_parser.OFPInstructionActions(
                ofp.OFPIT_APPLY_ACTIONS,
                actions=[
                    ofp_parser.OFPActionSetField(ip_dscp=0),
                    ofp_parser.OFPActionOutput(port=ofp.OFPP_IN_PORT)
                ]
            )
        ],
        priority=ofp.OFP_DEFAULT_PRIORITY-1
    ),
    ofp_parser.OFPFlowMod(
        datapath,
        command=command,
        out_port=ofp.OFPP_ANY,
        out_group=ofp.OFPG_ANY,
        match=ofp_parser.OFPMatch(
            eth_type=ether_types.ETH_TYPE_IP,
            ip_dscp=out_dscp(outport)
        ),
        instructions=[
            ofp_parser.OFPInstructionActions(
                ofp.OFPIT_APPLY_ACTIONS,
                actions=[
                    ofp_parser.OFPActionSetField(ip_dscp=0),
                    ofp_parser.OFPActionOutput(port=outport)
                ]
            )
        ],
        priority=ofp.OFP_DEFAULT_PRIORITY-2
    )]

class PBCE(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    _CONTEXTS = {
        'dpset': dpset.DPSet}

    def __init__(self, *args, **kwargs):
        super(PBCE, self).__init__(*args, **kwargs)

        self.CONF = cfg.CONF
        self.CONF.register_opts([
            cfg.IntOpt('lower', default=20),
            cfg.IntOpt('upper', default=30),
            cfg.FloatOpt('dc', default=0.5),
            cfg.FloatOpt('weight', default=0.5),
            cfg.FloatOpt('debounce', default=5.0),
            cfg.FloatOpt('timeout', default=10.0)])

        self.logger.info("parameters:\n"
                         " lower: %s\n"
                         " upper: %s\n"
                         " dc: %s\n"
                         " weight: %s\n"
                         " debounce: %s\n"
                         " timeout: %s",
                         self.CONF.lower,
                         self.CONF.upper,
                         self.CONF.dc,
                         self.CONF.weight,
                         self.CONF.debounce,
                         self.CONF.timeout)

        self.dpset = kwargs['dpset']
        self.topo = nx.DiGraph()
        self.host = {}

        self.evports = {}
        self.rewrite = None

        self.arp = {}
        self.debounce = {}

        self.monitor_thread = hub.spawn(self._monitor)

        self.prev_util = {}
        self.util = {}
        self.count_arrv = {}
        self.arrv = {}

    def ports(self, dpid):
        ofp = self.dpset.get(dpid).ofproto
        return [port.port_no for port in self.dpset.get_ports(dpid)
                if port.port_no < ofp.OFPP_MAX]

    @set_ev_cls(event.EventSwitchEnter)
    def _event_switch_enter_handler(self, ev):
        self.logger.info("switch enter: %s", ev.switch.dp.id)
        self.topo.add_node(ev.switch.dp.id)

        datapath = api.get_datapath(self, ev.switch.dp.id)
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        datapath.send_msg(
            ofp_parser.OFPFlowMod(
                datapath,
                command=ofp.OFPFC_ADD,
                match=ofp_parser.OFPMatch(),
                instructions=[
                    ofp_parser.OFPInstructionActions(
                        ofp.OFPIT_APPLY_ACTIONS,
                        actions=[
                            ofp_parser.OFPActionOutput(
                                port=ofp.OFPP_CONTROLLER,
                                max_len=ofp.OFPCML_NO_BUFFER
                            )
                        ]
                    )
                ],
                priority=1
            )
        )

    @set_ev_cls(event.EventSwitchLeave)
    def _event_switch_leave_handler(self, ev):
        self.logger.info("switch leave: %s", ev.switch.dp.id)
        self.topo.remove_node(ev.switch.dp.id)

    @set_ev_cls(event.EventLinkAdd)
    def _event_link_add_handler(self, ev):
        if ev.link.src.dpid in self.topo and ev.link.dst.dpid in self.topo:
            self.topo.add_edge(ev.link.src.dpid, ev.link.dst.dpid,
                pt=(ev.link.src.port_no,ev.link.dst.port_no))

    @set_ev_cls(event.EventLinkDelete)
    def _event_link_delete_handler(self, ev):
        if ev.link.src.dpid in self.topo and ev.link.dst.dpid in self.topo:
            self.topo.remove_edge(ev.link.src.dpid, ev.link.dst.dpid)

    @set_ev_cls(event.EventHostAdd)
    def _event_host_add_handler(self, ev):
        self.host[ev.host.mac] = (ev.host.port.dpid, ev.host.port.port_no)

    @set_ev_cls(ofp_event.EventOFPPortStatsReply, MAIN_DISPATCHER)
    def _port_stats_reply_handler(self, ev):
        msg = ev.msg
        datapath = msg.datapath
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        for stat in msg.body:
            k = (datapath.id, stat.port_no)
            cx_bytes = stat.rx_bytes+stat.tx_bytes
            dx_bytes = cx_bytes - self.prev_util.get(k, 0)
            self.prev_util[k] = cx_bytes
            self.util[k] = (self.CONF.weight*dx_bytes+
                            (1-self.CONF.weight)*self.util.get(k, 0))

    @set_ev_cls(ofp_event.EventOFPTableStatsReply, MAIN_DISPATCHER)
    def _table_stats_reply_handler(self, ev):
        msg = ev.msg
        datapath = msg.datapath
        dpid = datapath.id
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser
        active_count = next(stat.active_count for stat in msg.body
                            if stat.table_id == 0)

        with open(os.path.join("/result", str(dpid)), "a") as f:
            f.write(str(active_count) + "\n")

        for pt in self.ports(dpid):
            k = (dpid, pt)
            self.arrv[k] = (self.CONF.weight*self.arrv.get(k, 0)+
                            (1-self.CONF.weight)*self.count_arrv.get(k, 0))
            self.count_arrv[k] = 0

        if active_count < self.CONF.lower:
            evs = 0
            evport = None
            evarrv = float('inf')
            for pt in self.ports(dpid):
                k = (dpid, pt)
                if k not in self.evports: continue
                evs += 1
                if self.arrv.get(k, 0.0) < evarrv:
                    evport, evarrv = pt, self.arrv.get(k, 0.0)

            if evport is None:
                return

            dlport = self.evports.pop((dpid, evport))
            self.logger.info("%s: unevict port %s over %s",
                             dpid, evport, dlport)
            datapath.send_msg(
                eviction_rule(datapath, ofp.OFPFC_DELETE, evport, dlport))

            if evs == 1:
                hub.spawn(self._delete_backflow(datapath))

        elif self.CONF.upper < active_count:
            anyev = False
            evport = None
            evarrv = 0.0
            for pt in self.ports(dpid):
                k = (dpid, pt)
                if k in self.evports:
                    anyev = True
                    continue
                if evarrv <= self.arrv.get(k, 0.0):
                    evport, evarrv = pt, self.arrv.get(k, 0.0)

            if evport is None:
                return

            dlport = None
            dlutil = float('inf')
            for _,_,(pt,_) in self.topo.out_edges_iter(dpid, data='pt'):
                k = (dpid, pt)
                if self.util.get(k, 0.0) < dlutil:
                    dlport, dlutil = pt, self.util.get(k, 0.0)

            if dlport is None:
                return

            if not anyev:
                self.logger.info("%s: add backflow rules", dpid)
                for outport in [pt for pt in self.ports(dpid)]:
                    for msg in backflow_rules(datapath, ofp.OFPFC_ADD,
                                              outport):
                        datapath.send_msg(msg)

            self.logger.info("%s: evict port %s over %s", dpid, evport, dlport)

            self.evports[(dpid, evport)] = dlport
            datapath.send_msg(
                eviction_rule(datapath, ofp.OFPFC_ADD, evport, dlport))

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        msg = ev.msg
        in_port = msg.match['in_port']
        datapath = msg.datapath
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)

        if eth.ethertype == ether_types.ETH_TYPE_ARP:
            #Send arp packets out over all switches
            assert msg.buffer_id == ofp.OFP_NO_BUFFER
            arp_ = pkt.get_protocol(arp.arp)

            ctime = time.time()
            key = (arp_.hwtype, arp_.proto, arp_.hlen, arp_.plen, arp_.opcode,
                   arp_.src_mac, arp_.src_ip, arp_.dst_mac, arp_.dst_ip)
            if (ctime < self.arp.get(key, float('-inf')) + self.CONF.debounce):
                return

            self.arp.get(key, float('inf'))
            for dpid in self.topo:
                datapath = api.get_datapath(self, dpid)
                datapath.send_msg(
                    ofp_parser.OFPPacketOut(
                        datapath,
                        buffer_id=ofp.OFP_NO_BUFFER,
                        in_port=ofp.OFPP_CONTROLLER,
                        actions=[
                            ofp_parser.OFPActionOutput(ofp.OFPP_FLOOD)
                        ],
                        data=msg.data
                    )
                )

            self.arp[key] = time.time()
        elif eth.ethertype == ether_types.ETH_TYPE_IP:
            ipv4_ = pkt.get_protocol(ipv4.ipv4)
            dscp = ipv4_.tos >> 2
            if dscp != 0:
                for _,odpid,(ipt,_) in (
                    self.topo.out_edges_iter(datapath.id, data='pt')):
                    if ipt == in_port:
                        self.rewrite = (datapath.id, in_port)
                        msg.datapath = api.get_datapath(self, odpid)
                        replace_fields(ofp_parser, msg, in_port=dscp_in(dscp))
                        ipv4_.tos = ipv4_.tos & 3
                assert self.rewrite is not None
            self._packet_in_handler_short(ev)
            self.rewrite = None

    def _send(self, dpid, msg):
        datapath = api.get_datapath(self, dpid)
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        if isinstance(msg, ofp_parser.OFPFlowMod):
            k = (dpid, msg.match['in_port'])
            self.count_arrv[k] = 1 + self.count_arrv.get(k, 0)
            if k in self.evports:
                dlport = self.evports[k]
                for _,odpid,(ipt,opt) in (
                    self.topo.out_edges_iter(dpid, data='pt')):
                    if ipt == dlport:
                        dlport = None
                        rewrite_msg(datapath, msg, opt)
                        dpid = odpid
                        break
                assert dlport is None
        elif isinstance(msg, ofp_parser.OFPPacketOut):
            if self.rewrite is not None:
                dpid, export = self.rewrite
                rewrite_msg(datapath, msg, export)
        api.get_datapath(self, dpid).send_msg(msg)

    def _find_path(self, src_dpid, src_port_no, dst_dpid, dst_port_no):
        path = nx.shortest_path(self.topo, src_dpid, dst_dpid)

        def generator():
            out = (src_port_no, src_dpid)
            for swp, swn in zip(path, path[1:]):
                _outport, _inport = self.topo[swp][swn]['pt']
                yield out + (_outport,)
                out = (_inport, swn)
            yield out + (dst_port_no,)
        return list(generator())

    def _reverse_path(self, path):
        return [(dst, dpid, src) for (src, dpid, dst) in reversed(path)]

    def _packet_in_handler_short(self, ev):
        msg = ev.msg
        in_port = msg.match['in_port']
        datapath = msg.datapath
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)

        if eth.ethertype != ether_types.ETH_TYPE_IP:
            return
        ipv4_ =  pkt.get_protocol(ipv4.ipv4)

        if (ipv4_.proto == in_proto.IPPROTO_TCP):
            tcp_ = pkt.get_protocol(tcp.tcp)
            match = {'tcp_src': tcp_.src_port, 'tcp_dst': tcp_.dst_port}
            tsrc, tdst = tcp_.src_port, tcp_.dst_port
        elif (ipv4_.proto == in_proto.IPPROTO_UDP):
            udp_ = pkt.get_protocol(udp.udp)
            match = {'udp_src': udp_.src_port, 'udp_dst': udp_.dst_port}
            tsrc, tdst = udp_.src_port, udp_.dst_port
        else:
            match = {}
            tsrc, tdst = 0, 0

        if eth.dst not in self.host:
            return
        (dst_dpid, dst_port_no) = self.host[eth.dst]
        path1 = self._find_path(datapath.id, in_port, dst_dpid, dst_port_no)
        path2 = self._find_path(dst_dpid, dst_port_no, datapath.id, in_port)
        if hash(tuple(path1)) < hash(tuple(path2)): path = path1
        else: path = self._reverse_path(path2)

        self._send(datapath.id, ofp_parser.OFPPacketOut(
            datapath,
            buffer_id=msg.buffer_id,
            in_port=in_port,
            actions=[ofp_parser.OFPActionOutput(path[0][2])],
            data=msg.data if msg.buffer_id == ofp.OFP_NO_BUFFER else None
        ))

        ctime = time.time()
        key = (eth.src, eth.dst, ipv4_.src, ipv4_.dst, ipv4_.proto, tsrc, tdst)
        if ctime < self.debounce.get(key, float('-inf')) + self.CONF.debounce:
            return

        for in_port, dpid, out_port in path:
            datapath = api.get_datapath(self, dpid)
            self._send(dpid, ofp_parser.OFPFlowMod(
                datapath,
                command=ofp.OFPFC_ADD,
                match=ofp_parser.OFPMatch(
                    in_port=in_port,
                    eth_src=eth.src,
                    eth_dst=eth.dst,
                    eth_type=ether_types.ETH_TYPE_IP,
                    ipv4_src=ipv4_.src,
                    ipv4_dst=ipv4_.dst,
                    ip_dscp=0,
                    ip_proto=ipv4_.proto,
                    **match
                ),
                instructions=[
                    ofp_parser.OFPInstructionActions(
                        ofp.OFPIT_APPLY_ACTIONS,
                        actions=[ofp_parser.OFPActionOutput(out_port)]
                    )
                ],
                idle_timeout=self.CONF.timeout
            ))

        self.debounce[key] = ctime

    def _delete_backflow(self, datapath):
        self.logger.info("%s: delete backflow rules", datapath.id)
        ofp = datapath.ofproto
        hub.sleep(0.9*self.CONF.dc)
        for outport in [pt for pt in self.ports(datapath.id)]:
            for msg in backflow_rules(datapath, ofp.OFPFC_DELETE,
                                      outport):
                datapath.send_msg(msg)

    def _monitor(self):
        while True:
            for dpid in list(self.topo.nodes_iter()):
                datapath = api.get_datapath(self, dpid)
                if datapath is None: continue
                ofp = datapath.ofproto
                ofp_parser = datapath.ofproto_parser
                datapath.send_msg(ofp_parser.OFPPortStatsRequest(
                    datapath, 0, ofp.OFPP_ANY))
                datapath.send_msg(ofp_parser.OFPTableStatsRequest(datapath, 0))
            hub.sleep(self.CONF.dc)
