"""configurator for omnetpp adapter
"""

def configure(properties):
    """configurator function for omnetpp adapter

    Args:
        properties: configuration properties
    Returns:
        run args
    """
    return dict(
        command=f"./seed --sim-time-limit={properties['runtime']}s",
        volumes={
            properties["bundledir"]: {
                "bind": "/home/omnetpp/bundle",
                "mode": "ro"
            },
            properties["resultdir"]: {
                "bind": "/home/omnetpp/result",
                "mode": "rw"
            }
        }
    )
