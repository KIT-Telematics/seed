#!/usr/bin/python

from optparse import OptionParser
import xml.etree.ElementTree as ET

from mininet.clean import cleanup
from mininet.net import Mininet
from mininet.log import lg, LEVELS, info, error, setLogLevel
from mininet.link import TCLink
from subprocess import PIPE

from seed_topo import build_seed_topo, start_seed_topo
import seed_util as su
from seed_util import info, warn, debug
import time
import random
import sys
import re
import Queue as queue
from mininet.node import ( Controller, OVSController,
                           Ryu, NOX, RemoteController, findController,
                           DefaultController, NullController)


CONTROLLERS = { 'ref': Controller,
                'ovsc': OVSController,
                'nox': NOX,
                'remote': RemoteController,
                'ryu': Ryu,
                'none': NullController }



bundle = "/home/mininet/bundle/bundle.xml"


class EventObj:

    def __init__(self, parent, start, element):
        self.start = start + 0 if not element.get("start") else \
            su.convert_delay(element.get("start"), 'us')
        self.entry = element
        self.parent = parent


    def __lt__(self, other):
        return True if other.start > self.start else False

    def run(self):
            return False

    def _wait_for_start(self):
        while time.time()*1000000 < self.parent.simstart + self.start:
            debug("Current time: " + str(time.time()*1000000) + " - start of next event: " +
                  str(self.parent.simstart + self.start))
            time.sleep(0.001)

    def wait(self):
        return True


class BulkEvent (EventObj):

    def __init__(self, parent, start, element):
        EventObj.__init__(self, parent, start, element)

        self.sender = None
        self.receiver = None


    def run(self):

        debug("Starting bulk event")
        source = su.get_host(self.parent.net, self.entry.get("source"))
        destination = su.get_host(self.parent.net, self.entry.get("destination"))
        debug("source: " + source.name)
        debug("destination: " + destination.name)

        self._wait_for_start()

        # find unused receiver port (blacklist)
        port = random.randint(5000,10000)
        if not hasattr(destination , 'used_ports'): destination.used_ports = set()
        while port in destination.used_ports:
            port = random.randint(5000, 20000)
        destination.used_ports.add(port)

        cmnd = "iperf3 -1sJ -p " + str(port) + " > " + self.parent.logdir + \
               "/bulk-event_receiver-" + self.entry.get("destination") + "_id-" + str(self.parent.id) + ".iperf"
        debug("calling: " + cmnd + " on " + source.name)
        process = destination.popen(cmnd, shell=True)
        self.receiver = process

        # start sender
        cmnd = "iperf3 -c " + destination.IP() + " -Jp " + str(port)

        if self.entry.get("bandwidth"): cmnd += " -b " + str(su.convert_bw(self.entry.get("bandwidth"), 'bps'))
        if self.entry.get("duration"): cmnd += " -t " + str(su.convert_delay(self.entry.get("duration"), 's'))
        if self.entry.get("max-size"): cmnd += " -n " + str(su.convert_data(self.entry.get("max-size"), 'byte'))

        cmnd += " > " + self.parent.logdir + "/bulk-event_sender-" + source.name + "_id-" + str(
            self.parent.id) + ".iperf"
        debug("calling: " + cmnd + " on " + source.name)

        self.sender = source.popen(cmnd, shell=True)

        self.parent.id += 1

        return True

    def wait(self):
        if self.sender:
            self.sender.wait()
        debug("Primary process stopped")




class SeedMN:

    def __init__(self):
        self.parseArgs()
        if self.options.clean:
            cleanup()
            exit()
        bundle = self.options.bundle
        lg.setLogLevel(self.options.verbosity)
        random.seed(a=None)
        self.logdir = "/home/mininet/result"
        self.ports = dict()
        self.id = 0
        info("\n================================")
        info("Reading and building topology")
        self.net = Mininet(ipBase = self.options.ipbase, autoSetMacs= self.options.mac,
                           autoStaticArp = self.options.arp ,autoPinCpus= self.options.pin)
        build_seed_topo(self.net, self.options.bundle)
        info("\n================================")
        info("Creating event schedule")
        self.events = queue.PriorityQueue()
        self.create_events_schedule()
        info("\n================================")
        info("Starting up topology")
        start_seed_topo(self.net)
        info("\n================================")
        info("topology start done")
        time.sleep(30)
        self.simstart = time.time()*1000000
        info("\n================================")
        info("Executing Events")
        self.execute_events()
        info("\n================================")
        info("Event execution done --- exiting")
        time.sleep(10)
        for node in self.net.hosts:
            node.cmd("pkill iperf3")
        self.net.stop()


    def start_logging(self):
        for i in self.topo.namedinterfaces:
            if i.second.get("logging"):
                if i.second.get("logging") == "syns":
                    i.first.popen('tcpdump -i ' + i.first + ' "tcp[tcpflags] & (tcp-syn) != 0"  > ' + self.logdir + "/dump_if-" + i.first.get("name") + " bulk-event_receiver-" + self.entry.get("destination") + "_id-" + ".iperf")

    def create_events_schedule(self):
        xml = ET.parse(bundle)
        root = xml.getroot()
        schedule = root.find("schedule")
        for element in schedule.getchildren():
            self.parse_element(element)


    def parse_element(self, element, start = 0.0):
        if element.tag == "choice":
            self.parse_element(random.choice(list(element)), start)
        elif element.tag == "process":
            time = start
            if element.get("start"):
                time += su.convert_delay(element.get("start"), 'us')
            counter = int(element.get("max-repeat")) if element.get("max-repeat") else -1
            if element.get("fire"):
                match = re.match("interval\((.*)\)", str(element.get("fire")))
                if match and match.group(1):
                    while(time < self.options.duration and counter != 0):
                        for e in list(element):
                            self.parse_element(e, time)
                        counter -= 1
                        time += su.convert_delay(match.group(1), 'us')

                match = re.match("uniform\((.*),(.*)\)", str(element.get("fire")))
                if match and match.group(1):
                    while (time < self.options.duration and counter != 0):
                        for e in list(element):
                            self.parse_element(e, time)
                        counter -= 1
                        time += random.randrange(
                            su.convert_delay(match.group(1), 'us'),
                            su.convert_delay(match.group(2), 'us'))

                match = re.match("pareto\((.*),(.*)\)", str(element.get("fire")))
                if match and match.group(1):
                    while (time < self.options.duration and counter != 0):
                        for e in list(element):
                            self.parse_element(e, time)
                        counter -= 1
                        time += su.convert_delay(match.group(1), 'us') * \
                            random.paretovariate(float(match.group(2)))

                match = re.match("exp\((.*)\)", str(element.get("fire")))
                if match and match.group(1):
                    while (time < self.options.duration and counter != 0):
                        for e in list(element):
                            self.parse_element(e, time)
                        counter -= 1
                        time += random.expovariate(
                            1.0 / float(su.convert_delay(match.group(1), 'us')))

        elif element.tag == "bulk-event":
            self.events.put(BulkEvent(self, start, element))


            #self.events.put(BulkEvent(self, start, element))

        elif element.tag == "link-state-change-event":
            self.events.put(EventObj(self, start, element))
        else:
            warn("Unrecognized element in event schedule: Tag is " + element.tag)



    def execute_events(self):
        started = queue.Queue()
        while not self.events.empty():
            event = self.events.get()
            if event.run():
                started.put(event)

        info("All Processes started")

        while not started.empty():
            event = started.get()
            event.wait()
            del event

        info("All Processes finished - Done")


    def parseArgs( self ):
        """Parse command-line args and return options object.
           returns: opts parse options dict"""

        def addDictOption(opts, choicesDict, default, name, **kwargs):
            """Convenience function to add choices dicts to OptionParser.
               opts: OptionParser instance
               choicesDict: dictionary of valid choices, must include default
               default: default choice key
               name: long option name
               kwargs: additional arguments to add_option"""
            helpStr = ('|'.join(sorted(choicesDict.keys())) +
                       '[,param=value...]')
            helpList = ['%s=%s' % (k, v.__name__)
                        for k, v in choicesDict.items()]
            helpStr += ' ' + (' '.join(helpList))
            params = dict(type='string', default=default, help=helpStr)
            params.update(**kwargs)
            opts.add_option('--' + name, **params)

        desc = ( "The %prog utility creates Mininet network from the"
                 "command line. It can create parametrized topologies,"
                 "invoke the Mininet CLI, and run tests." )

        usage = ( '%prog [options]'
                  '(type %prog -h for details)' )

        opts = OptionParser( description=desc, usage=usage )

        addDictOption( opts, CONTROLLERS, [], 'controller', action='append' )

        opts.add_option('--clean', '-c', action='store_true',
                        default=False, help='clean and exit')
        opts.add_option('--bundle', '-b', type='string',
                        default="/home/mininet/bundle/bundle.xml", help='bundle xml file to load')
        opts.add_option( '--ipbase', '-i', type='string', default='10.0.0.0/8',
                         help='base IP address for hosts' )
        opts.add_option( '--mac', action='store_true',
                         default=False, help='automatically set host MACs' )
        opts.add_option( '--arp', action='store_true',
                         default=False, help='set all-pairs ARP entries' )
        opts.add_option( '--verbosity', '-v', type='choice',
                         choices=LEVELS.keys(), default = 'info',
                         help = '|'.join( LEVELS.keys() )  )
        opts.add_option('--duration', type='string', default=100,
                       help='Duration of simulation in sec')
        opts.add_option('--controllerIP', type='string', default="127.0.0.1",
                       help='IP of controller')
        opts.add_option('--logDir', type='string', default="/home/mininet/result",
                        help='dir to log to')
        opts.add_option( '--pin', action='store_true',
                         default=False, help="pin hosts to CPU cores "
                         "(requires --host cfs or --host rt)" )
        self.options, self.args = opts.parse_args()

        # We don't accept extra arguments after the options
        if self.args:
            opts.print_help()
            exit()

if __name__ == '__main__':
    setLogLevel('info')

    try:
        SeedMN()
    except Exception:
        # Print exception
        type_, val_, trace_ = sys.exc_info()
        errorMsg = ("-" * 80 + "\n" +
                    "Caught exception. Cleaning up...\n\n" +
                    "%s: %s\n" % (type_.__name__, val_) +
                    "-" * 80)
        error(errorMsg)
        # Print stack trace to debug log
        import traceback

        stackTrace = traceback.format_exc()
        debug(stackTrace)
        cleanup()
