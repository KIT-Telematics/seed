#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import socket
import subprocess
import sys
import time

ADD_FORWARDING_CMD_1 = "sudo iptables -t nat -A PREROUTING -i controller1 -j DNAT --to-destination {0}"
ADD_FORWARDING_CMD_2 = "sudo iptables -t nat -A POSTROUTING -d {0} -j MASQUERADE"

def main():
    options = parse_options()
    controllers = sum(options.controllers, [])
    ipAddresses = []
    i = 0

    time.sleep(2)
    print("[Startup]")
    for controller in controllers:
        try:
            ip = socket.gethostbyname(controller)
            print("Controller #{0}: {1} (IP: {2})".format(i, controller, ip))
            ipAddresses.append(ip)
            i += 1
        except socket.gaierror, err:
            print("Error: cannot resolve IP address for: " + controller)
            sys.exit(1)
            return 1

    processToWaitFor = []
    for ip in ipAddresses:
        print("Adding forwarding rules for {0}".format(ip))

        cmd1 = ADD_FORWARDING_CMD_1.format(ip)
        print(cmd1)
        process1 = subprocess.Popen(cmd1, shell=True)
        processToWaitFor.append(process1)

        process1.wait()

        cmd2 = ADD_FORWARDING_CMD_2.format(ip)
        print(cmd2)
        process2 = subprocess.Popen(cmd2, shell=True)
        processToWaitFor.append(process2)

        process2.wait()

        print("Adding forwarding rules for {0} - DONE".format(ip))

    exit_codes = [p.wait() for p in processToWaitFor]
    print(exit_codes)
    print("[Simulator]")
    print("Starting Simulator")
    simulatorCmd = "cd /home/ns3/ns-3.26/ && ./waf --run \"" + options.ns3params + "\""
    # simulatorCmd = "sleep inf"
    print(simulatorCmd)
    process = subprocess.Popen(simulatorCmd, shell=True)
    process.wait()
    print("Simulator - DONE")
    # x = subprocess.Popen("sleep inf", shell=True)
    # x.wait()

    print("[Cleanup]")
    for ip in ipAddresses:
        print("Removing forwarding rules for {0}".format(ip))
        print("Removing forwarding rules for {0} - DONE".format(ip))

    sys.exit(0)
    return 0

def parse_options():
    parser = argparse.ArgumentParser()

    parser.add_argument(
            "-c",
            help="Names of controllers to resolve and forward",
            dest="controllers",
            nargs="*",
            action="append")

    parser.add_argument(
            "--ns3",
            help="Params to pass to ns3",
            dest="ns3params",
            type=str,
            default="")

    options = parser.parse_args()

    return options

if __name__ == "__main__":
    main()
