from mininet.link import TCLink
from mininet.node import RemoteController, OVSSwitch
import seed_util as su
from seed_util import info, warn, debug
import socket
import xml.etree.ElementTree as ET
from mininet.topo import Topo
from itertools import groupby

bundle = "/home/mininet/bundle/bundle.xml"


def add_groups(tree, d):
    for group in tree.findall("group"):
        d[group.get('name')] = set()

class SeedTopo (Topo):


    def __init__(self, sample_controllers = False, bundle=bundle):
        Topo.__init__(self)
        self.nodegroups = dict()
        self.linkgroups = dict()
        self.interfacegroups = dict()
        self.node_idx = dict()
        self.idx_node = dict()

        self.switches_idx = dict()
        self.hosts_idx = dict()
        self.controllers = []
        self.controllers_idx = dict()

        self.interfaces = dict()

        self.links_idx = dict()

        info("reading Seed Topology")
        xml = ET.parse(bundle)
        root = xml.getroot()

        info("creating groups")
        add_groups(root.find("topology").find("nodegroups"), self.nodegroups)
        add_groups(root.find("topology").find("linkgroups"), self.linkgroups)
        add_groups(root.find("topology").find("interfacegroups"), self.interfacegroups)

        info("creating nodes")
        nodes = root.find("topology").find("nodes").findall("node")
        for idx, node in enumerate(nodes):
            _idx = "n" + str(idx)
            self.node_idx[node.get("name")] = _idx
            self.idx_node[_idx] = node.get("name")
            node.idx = _idx

            if node.get("groups"):
                for group in node.get("groups").split(" "):
                    assert group in self.nodegroups
                    self.nodegroups[group].add(node)

            for nic in node.findall("interface"):
                self.interfaces[nic.get("name")] = (_idx, nic)

            if node.get("type") == "ofSwitch":
                self.switches_idx[_idx] = (self.addSwitch(_idx, cls=OVSSwitch),node)
            elif node.get("type") == "genericSwitch":
                self.switches_idx[_idx] = (self.addSwitch(_idx, failMode="standalone"),node)
            elif node.get("type") == "host":
                self.hosts_idx[_idx] = self.addHost(_idx)
            elif node.get("type") == "ofController":
                if not sample_controllers:
                    self.controllers_idx[_idx] = True
                    continue
                debug("Looking up " + str(self.idx_node[_idx]))
                try:
                    c = RemoteController(node.get("name"), ip=socket.gethostbyname(node.get("controller")), port=6633)
                    self.controllers.append(c)
                    self.controllers_idx[_idx] = c
                except:
                    raise AssertionError("Controller name " + node.get("controller") + " can not be resolved!")

            else:
                raise AssertionError("Node " + node.get("name") + " has an usupported type")

        info("creating links")
        for link in root.find("topology").find("links").findall("link"):
            params = dict()
            if (self.interfaces[link.get("a")][1].get('type') == 'control' or
                        self.interfaces[link.get("b")][1].get('type') == 'control' or
                        self.controllers_idx.get(self.interfaces[link.get("a")][0]) or
                        self.controllers_idx.get(self.interfaces[link.get("b")][0])):
                continue

            if link.get("bandwidth"):
                params.update(
                    dict(use_htb=True, bw=int(su.convert_bw(link.get("bandwidth"), 'Mbps'))))
            if False:#link.get("delay"):
                params.update(
                    dict(delay=str(int(su.convert_delay(link.get("delay"), 'ms')))))
            if link.get("buffer"):
                params.update(
                    dict(max_queue_size=(su.convert_data(link.get("buffer"), 'p'))))

            self.links_idx[link.get("name")] = self.addLink(
                self.interfaces[link.get("a")][0], self.interfaces[link.get("b")][0],
                cls=TCLink, **params)

            debug(link.get("name") + " created")


topos = { 'seedtopo': ( lambda: SeedTopo() ) }


def build_seed_topo(net, controller=None):
    "Create custom topo."

    t = SeedTopo(sample_controllers=True)
    net.topo = t
    net.controllers = t.controllers if not controller else controller
    net.build()

def start_seed_topo(net):
    for cont in net.topo.controllers:
        cont.checkListening()

    info('*** Starting %s switches\n' % len(net.switches))
    for idx, sw in enumerate(net.switches):
        type = net.topo.switches_idx[sw.name][1].get("type")
        if type == "ofSwitch":
            clist = [net.topo.controllers_idx[net.topo.node_idx[c]]
                         for c in net.topo.switches_idx[sw.name][1].get("controller").split(" ")]
            debug("Starting OFSwitch " + sw.name + " with controller list " + str(clist))
            sw.start(clist)
        elif type == "genericSwitch":
            debug("Starting genericSwitch " + sw.name)
            sw.start([])
        else:
            AssertionError("A switch was not started due to an unsupported type")

    started = {}
    success = OVSSwitch.batchStartup(net.switches)
    started.update({s: s for s in success})

    if net.waitConn:
        net.waitConnected()
