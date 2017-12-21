from pox.core import core
from pox.lib.packet.ethernet import ethernet
from pox.lib.recoco import Timer
from pox.lib.revent import EventMixin
from pox.openflow import PacketIn
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpid_to_str

log = core.getLogger()

def tos_in(tos):
    port = tos>>2
    assert 0 < port and port < 32
    return port

def in_tos(port):
    assert 0 < port and port < 32
    return port<<2

def tos_out(tos):
    port = (tos-128)>>2
    assert 0 < port and port < 32
    return port

def out_tos(port):
    assert 0 < port and port < 32
    return (port<<2)+128

def rewrite_msg(msg, export):
    assert msg.header_type in {of.OFPT_FLOW_MOD, of.OFPT_PACKET_OUT}
    if msg.header_type == of.OFPT_FLOW_MOD:
        assert msg.match.in_port is not None
        msg.match.nw_tos = in_tos(msg.match.in_port)
        msg.match.in_port = export
    if msg.header_type == of.OFPT_PACKET_OUT:
        msg.in_port = export
    oacts = [action for action in msg.actions
             if action.type == of.OFPAT_OUTPUT]
    assert len(oacts) == 1
    msg.actions.insert(0, of.ofp_action_nw_tos(nw_tos=out_tos(oacts[0].port)))
    oacts[0].port = of.OFPP_IN_PORT

def eviction_rule(evport, dlport):
    return of.ofp_flow_mod(
        match=of.ofp_match(
            in_port=evport,
            dl_type=ethernet.IP_TYPE,
            nw_tos=0,
        ),
        actions=[
            of.ofp_action_nw_tos(nw_tos=in_tos(evport)),
            of.ofp_action_output(port=
                of.OFPP_IN_PORT if evport == dlport else dlport)
        ],
        priority=of.OFP_DEFAULT_PRIORITY-1
    )

def backflow_rules(outport):
    return [of.ofp_flow_mod(
        match=of.ofp_match(
            in_port=_i,
            dl_type=ethernet.IP_TYPE,
            nw_tos=out_tos(outport)
        ),
        actions=[
            of.ofp_action_nw_tos(nw_tos=0),
            of.ofp_action_output(port=_o)
        ],
        priority=of.OFP_DEFAULT_PRIORITY-_p
    ) for _i, _o, _p in [(outport, of.OFPP_IN_PORT, 0), (None, outport, 1)]]

class pbce(EventMixin):
    prev_util = {}
    util = {}
    count_arrv = {}
    arrv = {}
    evports = {}
    rewrite = None

    _eventMixin_events = set([
        PacketIn
    ])

    def __init__(self, lower, upper, dc, weight):
        self.lower = int(lower)
        self.upper = int(upper)
        self.dc = int(dc)
        self.weight = float(weight)
        core.listen_to_dependencies(self)
        Timer(self.dc, self._handle_timer, recurring=True)

    def _handle_timer(self):
        for dpid in core.topo.graph.nodes_iter():
            for _,_,(pt,_) in core.topo.graph.out_edges_iter(dpid, data='pt'):
                msg = of.ofp_stats_request()
                msg.body = of.ofp_port_stats_request()
                msg.body.port_no = pt
                core.openflow.getConnection(dpid).send(msg)
        for dpid in core.topo.graph.nodes_iter():
            msg = of.ofp_stats_request()
            msg.body = of.ofp_table_stats_request()
            core.openflow.getConnection(dpid).send(msg)

    def _handle_backflow(self, conn):
        log.debug("Uninstalling backflow rules for %s", dpid_to_str(conn.dpid))
        for outport in [pt for pt in conn.ports.keys()
                        if pt <= of.OFPP_MAX]:
            for msg in backflow_rules(outport):
                msg.command = of.OFPFC_DELETE_STRICT
                conn.send(msg)

    def _handle_openflow_PacketIn(self, event):
        connection = event.connection
        dpid = event.dpid
        inport = event.port
        packet = event.parsed
        _ipv4 = packet.find('ipv4')
        if not _ipv4:
            return
        if _ipv4.tos != 0:
            for _,odpid,(ipt,_) in (
                core.topo.graph.out_edges_iter(dpid, data='pt')):
                if ipt == inport:
                    self.rewrite = (dpid, inport)
                    ofp = event.ofp.clone()
                    ofp.in_port = tos_in(_ipv4.tos)
                    _ipv4.tos = 0
                    ofp.data = packet
                    event = PacketIn(core.openflow.getConnection(odpid), ofp)
                    break
            assert self.rewrite is not None
        self.raiseEventNoErrors(event)
        self.rewrite = None

    def _handle_openflow_TableStatsReceived(self, event):
        dpid = event.connection.dpid
        conn = core.openflow.getConnection(dpid)
        active_count = next(stat.active_count for stat in event.stats
                            if stat.table_id == 0)

        for pt in conn.ports.keys():
            if of.OFPP_MAX < pt: continue
            k = (dpid, pt)
            self.arrv[k] = (self.weight*self.arrv.get(k, 0)+
                            (1-self.weight)*self.count_arrv.get(k, 0))
            self.count_arrv[k] = 0

        if active_count < self.lower:
            evs = 0
            evport = None
            evarrv = float('inf')
            for pt in conn.ports.keys():
                k = (dpid, pt)
                if of.OFPP_MAX < pt: continue
                if (dpid, pt) not in self.evports: continue
                evs += 1
                if self.arrv.get(k, 0.0) < evarrv:
                    evport, evarrv = pt, self.arrv.get(k, 0.0)

            if evport is None:
                return

            log.debug("Unevicting port %s for %s", evport, dpid_to_str(dpid))
            dlport = self.evports.pop((dpid, evport))
            msg = eviction_rule(evport, dlport)
            msg.command = of.OFPFC_DELETE_STRICT
            conn.send(msg)

            if evs == 1:
                Timer(0.5*self.dc, self._handle_backflow, args=[conn])

        elif self.upper < active_count:
            anyev = False
            evport = None
            evarrv = 0.0
            for pt in conn.ports.keys():
                k = (dpid, pt)
                if of.OFPP_MAX < pt: continue
                if (dpid, pt) in self.evports:
                    anyev = True
                    continue
                if evarrv <= self.arrv.get(k, 0.0):
                    evport, evarrv = pt, self.arrv.get(k, 0.0)

            if evport is None:
                return

            dlport = None
            dlutil = float('inf')
            for _,_,(pt,_) in core.topo.graph.out_edges_iter(dpid, data='pt'):
                k = (dpid, pt)
                if of.OFPP_MAX < pt: continue
                if self.util.get(k, 0.0) < dlutil:
                    dlport, dlutil = pt, self.util.get(k, 0.0)

            if dlport is None:
                return

            if not anyev:
                log.debug("Installing backflow rules for %s",
                    dpid_to_str(dpid))
                for outport in [pt for pt in conn.ports.keys()
                                if pt <= of.OFPP_MAX]:
                    for msg in backflow_rules(outport):
                        conn.send(msg)

            log.debug("Evicting port %s to %s for %s",
                evport, dlport, dpid_to_str(dpid))
            self.evports[(dpid, evport)] = dlport
            conn.send(eviction_rule(evport, dlport))

    def _handle_openflow_PortStatsReceived(self, event):
        if len(event.stats) != 1: return
        k = (event.connection.dpid, event.stats[0].port_no)
        cx_bytes = event.stats[0].rx_bytes+event.stats[0].tx_bytes
        dx_bytes = cx_bytes - self.prev_util.get(k, 0)
        self.prev_util[k] = cx_bytes
        self.util[k] = self.weight*dx_bytes+(1-self.weight)*self.util.get(k, 0)

    def _handle_openflow_PortStatus(self, event):
        k = (event.dpid, event.port)
        if event.deleted:
            self.prev_util.pop(k, None)
            self.util.pop(k, None)
            self.count_arrv.pop(k, None)
            self.arrv.pop(k, None)
            ev = self.evports.pop(k, None)
            if ev is not None:
                # TODO uninstall eviction rule
                pass

    def send(self, dpid, msg):
        if msg.header_type == of.OFPT_FLOW_MOD:
            k = (dpid, msg.match.in_port)
            if k in self.evports:
                dlport = self.evports[k]
                for _,odpid,(ipt,opt) in (
                    core.topo.graph.out_edges_iter(dpid, data='pt')):
                    if ipt == dlport:
                        dlport = None
                        rewrite_msg(msg, opt)
                        dpid = odpid
                        break
                assert dlport is None
            log.debug("Installing flow for %s", dpid_to_str(dpid))
        elif msg.header_type == of.OFPT_PACKET_OUT:
            if self.rewrite is not None:
                dpid, export = self.rewrite
                rewrite_msg(msg, export)
            log.debug("Sending packet to %s", dpid_to_str(dpid))
        core.openflow.getConnection(dpid).send(msg)

def launch(lower=100, upper=200, dc=1, weight=0.75):
    core.registerNew(pbce, lower=lower, upper=upper, dc=dc, weight=weight)
