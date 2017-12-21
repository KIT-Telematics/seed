import networkx as nx
from pox.core import core
from pox.lib.revent import EventMixin

log = core.getLogger()

class topo(EventMixin):
    graph = nx.DiGraph()

    def __init__(self):
        core.listen_to_dependencies(self, listen_args={
            'openflow':{'priority':0}})

    def _handle_openflow_discovery_LinkEvent(self, event):
        l = event.link
        sw1 = l.dpid1
        sw2 = l.dpid2
        pt1 = l.port1
        pt2 = l.port2

        if event.added:
            assert self.graph.has_node(sw1) and self.graph.has_node(sw2)
            self.graph.add_edge(sw1,sw2,pt=(pt1,pt2))
        if event.removed:
            try:
                self.graph.remove_edge(sw1,sw2)
            except:
                log.warning("remove edge error")

    def _handle_openflow_ConnectionUp(self, event):
        sw = event.dpid
        self.graph.add_node(sw)

    def _handle_openflow_ConnectionDown(self, event):
        sw = event.dpid
        try:
            self.graph.remove_node(sw)
        except:
            log.warning("remove node error")

    def query(self, inport, sw1, sw2, outport):
        try:
            path = nx.shortest_path(self.graph, sw1, sw2)
        except nx.NetworkXNoPath:
            return None

        def generator():
            out = (inport, sw1)
            for swp, swn in zip(path, path[1:]):
                _outport, _inport = self.graph[swp][swn]['pt']
                yield out + (_outport,)
                out = (_inport, swn)
            yield out + (outport,)

        return list(generator())

def launch():
    core.registerNew(topo)
