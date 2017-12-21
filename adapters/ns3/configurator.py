"""configurator for ns-3 adapter
"""

def setup(properties):
    controllers = []
    for name, props in properties["controllers"].items():
        controllers.append(name)
    tmpControllers = [ " -c " + str(controller) for controller in controllers]
    controllerRawStr =  "".join(tmpControllers)
    # print(controllerRawStr)
    # ns3params = "scratch/seed "\
    #     "--bundle=/home/ns3/bundle/bundle.xml " \
    #     "-o=/home/ns3/result/ " \
    #     "--runtime=" + str(properties['runtime']) + " " \
    #     "--randomSeed=" + str(properties['seed'])
    ns3params = "scratch/ofswitch13-external-controller"
    RUN_CMD = "/home/ns3/helper/ns3starter.py {0} --ns3 \"{1}\"".format(controllerRawStr, ns3params)
    print("RUN: " + RUN_CMD)
    return RUN_CMD



def configure(properties):
    """configurator function for ns-3 adapter

    Args:
        properties: configuration properties
    Returns:
        run args
    """
    RUN_CMD = setup(properties)
    return dict(
        # command="sleep inf",
        command=RUN_CMD,
        volumes={
            properties["helperdir"]: {
                "bind": "/home/ns3/helper",
                "mode": "ro"
            },
            properties["bundledir"]: {
                "bind": "/home/ns3/bundle",
                "mode": "ro"
            },
            properties["scratchdir"]: {
                "bind": "/home/ns3/ns-3.26/scratch",
                "mode": "ro"
            },
            properties["seeddir"]: {
                "bind": "/home/ns3/ns-3.26/src/seed",
                "mode": "ro"
            },
            properties["resultdir"]: {
                "bind": "/home/ns3/result",
                "mode": "rw"
            }
        }
    )
