from pox.core import core
from pox.lib.recoco import Timer
from pox.lib.revent import EventMixin
import pox.openflow.libopenflow_01 as of
import time

log = core.getLogger()

class short(EventMixin):
    def __init__(self, timeout):
        self.debounce = {}
        self.timeout = int(timeout)
        core.listen_to_dependencies(self)
        Timer(self.timeout, self._handle_timer, recurring=True)

    def _handle_timer(self):
        rmdebounce = {m for m, t in self.debounce.items()
                      if t + self.timeout < time.time()}
        for m in rmdebounce:
            del self.debounce[m]

    def _handle_pbce_PacketIn(self, event):
        dpid = event.connection.dpid
        inport = event.port
        packet = event.parsed
        if not packet.parsed:
            log.warning("%i: ignoring unparsed packet", dpid)
            return
        _ipv4 = packet.find('ipv4')
        if not _ipv4:
            return

        dst = core.host.query(_ipv4.dstip)
        if not dst:
            return
        path = core.topo.query(inport, dpid, dst[0], dst[1])
        if not path:
            return

        match = of.ofp_match.from_packet(packet)

        ctime = time.time()
        if self.debounce.get(match, float('-inf')) + self.timeout < ctime:
            msg = of.ofp_flow_mod(
                match=match.clone(),
                idle_timeout=self.timeout
            )
            for _inport, _dpid, _outport in path:
                msg.match.in_port = _inport
                msg.actions = [of.ofp_action_output(port=_outport)]
                core.pbce.send(_dpid, msg.clone())
            self.debounce[match] = ctime

        msg = of.ofp_packet_out(
            in_port=inport,
            data=event.ofp,
            action=[of.ofp_action_output(port=path[0][2])]
        )
        core.pbce.send(dpid, msg)

def launch(timeout=1):
    core.registerNew(short, timeout=timeout)
