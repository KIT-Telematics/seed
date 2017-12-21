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

class pbce_static(EventMixin):
    evports = {}
    rewrite = None

    _eventMixin_events = set([
        PacketIn
    ])

    def __init__(self):
        core.listen_to_dependencies(self)

    def _handle_openflow_ConnectionUp(self, event):
        conn = event.connection
        dpid = conn.dpid
        if not self.evports:
            self.evports[(dpid, 1)] = 2
            self.evports[(dpid, 2)] = 2
            msg = eviction_rule(1, 2)
            conn.send(msg)
            msg = eviction_rule(2, 2)
            conn.send(msg)
            for port in {1, 2}:
                for msg in backflow_rules(port):
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

def launch():
    core.registerNew(pbce2)
