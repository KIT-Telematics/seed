from pox.core import core
from pox.lib.packet.arp import arp
from pox.lib.revent import EventMixin

"""""
Keeps a Map of all over ARP seen IPs and the dpids they were seen on and the inport
host_map = {"IP": (dpid, inport)
"""

log = core.getLogger()

class host(EventMixin):
    host_map = {}

    def __init__(self):
        core.listen_to_dependencies(self)

    def _handle_openflow_PacketIn(self, event):
        dpid = event.connection.dpid
        inport = event.port
        packet = event.parsed
        if not packet.parsed:
            log.warning("%i: ignoring unparsed packet", dpid)
            return
        if not core.openflow_discovery.is_edge_port(dpid, inport):
            log.debug("ignore packetIn at switch port: %i %i", dpid, inport)
            return
        a = packet.find('arp')
        if not a:
            return
        if (a.prototype == arp.PROTO_TYPE_IP and
            a.hwtype == arp.HW_TYPE_ETHERNET and
            a.protosrc != 0):
            log.debug("Arp received, creating record: " + str((a.protosrc,dpid, inport, a.hwsrc)))
            self.host_map[a.protosrc] = (dpid, inport, a.hwsrc)

    def query(self, protosrc):
        return self.host_map.get(protosrc)

def launch():
    core.registerNew(host)
