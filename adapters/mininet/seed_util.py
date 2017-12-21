import re
import mininet.log
import random

bw_units = {"Gbps": 1000000000, "Mbps": 1000000, "Kbps": 1000}
data_units = {"Gbit": 1000000000, "Mbit": 1000000, "Kbit": 1000, "bit": 1, "Gbyte": 8000000000, "Mbyte": 8000000, "Kbyte": 8000, "byte": 8, "p":12000}
delay_units = {"us": 1, "ms": 1000, "s":1000000}

def info(msg):
    mininet.log.info(str(msg) + '\n')

def warn(msg):
    mininet.log.warn(str(msg) + '\n')

def err(msg):
    mininet.log.error(str(msg) + '\n')

def debug(msg):
    mininet.log.debug(str(msg) + '\n')


def convert_bw(bwStr, target):
    if not bwStr: return None
    match = re.match(r"^([0-9]*\.?[0-9]*)\s*(Gbps|Mbps|Kbps|bps)$", str(bwStr))

    if match:
        return int(float(match.group(1)) * bw_units[match.group(2)]/bw_units["Mbps"])
    else:
        warn("Warning " + bwStr + " does not fit the specified unit format")
        return int(bwStr)

def convert_delay(delayStr, target):
    if not delayStr: return None
    match = re.match(r"^([0-9]*\.?[0-9]*)\s*(ms|us|s)$", str(delayStr))
    if match:
        return float(match.group(1)) * delay_units[match.group(2)] /delay_units[target]
    else:
        warn("Warning " + delayStr + " does not fit the specified unit format")
        return float(delayStr)

def convert_data(dataStr, target):
    match = re.match(r"^([0-9]*\.?[0-9]*)\s*(Gbit|Mbit|Kbit|bit|Gbyte|Mbyte|Kbyte|byte|p)$", str(dataStr))
    if match:
        return int(float(match.group(1) ) * data_units[match.group(2)])/data_units[target]
    else:
        warn("Warning " + dataStr + " does not fit the specified unit format")
        return int(dataStr)


def get_host(net,string):
    if string == "sample(host)":
        return random.choice(net.hosts)
    match = re.match("sample\((.*)\)", str(string))
    try:
        if match:
            host = random.choice(list(net.topo.nodegroups[match.group(1)]))
            return net.nameToNode[net.topo.node_idx[host.get("name")]]
        else:
            return net.nameToNode[net.topo.node_idx[string]]
    except KeyError:
        AssertionError ("Node " + string + "could not be found!")
