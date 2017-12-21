import operator as op
import xml.etree.ElementTree as ET
import networkx as nx

PORT_TYPE_MULTIPLIER = 10000
SWITCH_ID_MULTIPLIER = 100000

PORT_MAP_FILENAME = "data/port_map.txt"
TOPO_FILENAME = "data/backbone_topology.tf"

WAN_HOSTS = {32768+10, 32768+20, 32768+111, 32768+125, 32768+150, 32768+200}

def load_nodes(filename):
    nodes = {}
    f = open(filename, 'r')
    for line in f:
        if not line.startswith("$") and line != "":
            (nid, iid) = node_port(int(line.strip().split(":")[1]))
            if nid not in nodes.keys():
                nodes[nid] = set()
            if iid not in nodes[nid]:
                nodes[nid].add(iid)
    f.close()
    return nodes

def load_links(filename):
    links = set()
    f = open(filename, 'r')
    for line in f:
        if line.startswith("link"):
            tokens = line.split('$')
            src = node_port(int(tokens[1].strip('[]')))
            dst = node_port(int(tokens[7].strip('[]')))
            links.add(src + dst)
    f.close()
    return links

def node_port(arg):
    return (arg // SWITCH_ID_MULTIPLIER,
            arg % PORT_TYPE_MULTIPLIER)

def name_node(node):
    return f'n{node}'

def name_itfc(node, itfc):
    return f'n{node}i{itfc}'

def name_link(src_node, src_itfc, dst_node, dst_itfc):
    src_itfc_name = name_itfc(src_node, src_itfc)
    dst_itfc_name = name_itfc(dst_node, dst_itfc)
    return f'l{src_itfc_name}{dst_itfc_name}'

def add_node_port(g, nid, iid, control=False):
    g.add_node((nid, iid), t='i', type='control' if control else 'data')
    g.add_edge(nid, (nid, iid), t='c')

def preprocess(nodes_map, links_set):
    g = nx.Graph()

    g.add_node(65536, t='n', groups="controller")
    add_node_port(g, 65536, 0)
    g.add_node(0, t='n', groups="switch")
    add_node_port(g, 0, 65536)
    g.add_edge((65536, 0), (0, 65536), t='l')

    for nid, iids in nodes_map.items():
        add_node_port(g, 0, nid)
        g.add_node(nid, t='n', groups="openflow")
        add_node_port(g, nid, 0, control=True)
        g.add_edge((0, nid), (nid, 0), t='l')
        for iid in iids:
            add_node_port(g, nid, iid)
    for src_nid, src_iid, dst_nid, dst_iid in links_set:
        g.add_edge((src_nid, src_iid), (dst_nid, dst_iid), t='l')

    his = set()
    eis = set()

    for nid, dat in g.nodes_iter(data=True):
        if dat['t'] == 'i':
            if g.degree(nid) == 1:
                his.add(nid)
            elif g.degree(nid) > 2:
                eis.add(nid)

    max_nid = 16384
    for nid in eis:
        nnids = set()
        nnids.add(nid)
        for nnid in g.neighbors(nid):
            if g.node[nnid]['t'] == 'i':
                nnids.add(nnid)

        for _, nnid, t in set(g.edges_iter(nid, 't')):
            if t == 'l':
                g.remove_edge(nid, nnid)

        g.add_node(max_nid, t='n', groups="switch")

        for iid, nnid in enumerate(nnids):
            add_node_port(g, max_nid, iid)
            g.add_edge(nnid, (max_nid, iid), t='l')

        max_nid += 1

    max_nid = 32768
    for nid in his:
        group = "host-wan" if max_nid in WAN_HOSTS else "host"
        g.add_node(max_nid, t='n', groups=group)
        add_node_port(g, max_nid, 0)
        g.add_edge(nid, (max_nid, 0), t='l')
        max_nid += 1

    nodes_map.clear()
    links_set.clear()

    for nid, dat in g.nodes_iter(data=True):
        if dat['t'] == 'n':
            nodes_map[nid] = {
                't': dat['groups'],
                'p': {}
            }

    for nid, dat in g.nodes_iter(data=True):
        if dat['t'] == 'i':
            nodes_map[nid[0]]['p'][nid[1]] = dat['type']

    for src, dst, t in g.edges_iter(data='t'):
        if t == 'l':
            links_set.add(src + dst)

def create_xml():
    nodes_map = load_nodes(PORT_MAP_FILENAME)
    links_set = load_links(TOPO_FILENAME)
    preprocess(nodes_map, links_set)

    bundle = ET.Element('bundle')
    bundle.set("name", "stanford backbone")

    bundle.append(ET.parse('data/parameters.xml').getroot())
    #bundle.append(ET.parse('data/data.xml').getroot())

    topology = ET.parse('data/topology.xml').getroot()
    bundle.append(topology)

    nodes = ET.SubElement(topology, 'nodes')
    for (nid, iids) in sorted(nodes_map.items(), key = op.itemgetter(0)):
        node = ET.SubElement(nodes, 'node')
        node.set('name', name_node(nid))
        node.set('groups', iids['t'])

        for iid in sorted(iids['p']):
            itfc = ET.SubElement(node, 'interface')
            itfc.set('name', name_itfc(nid, iid))
            if iids['p'][iid] == 'control':
                itfc.set('type', 'control')

    links = ET.SubElement(topology, 'links')
    for (src_nid, src_iid, dst_nid, dst_iid) in links_set:
        link = ET.SubElement(links, 'link')
        link.set('name', name_link(src_nid, src_iid, dst_nid, dst_iid))
        link.set('groups', 'default')
        link.set('a', name_itfc(src_nid, src_iid))
        link.set('b', name_itfc(dst_nid, dst_iid))

    bundle.append(ET.parse('data/schedule-simple.xml').getroot())
    #bundle.append(ET.parse('data/schedule.xml').getroot())

    ET.ElementTree(bundle).write('bundle.xml')
    #print(minidom.parseString(ET.tostring(bundle)).toprettyxml(indent="\t"))

create_xml()
