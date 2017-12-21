"""configurator for mininet adapter
"""

def configure(properties):
    """configurator function for mininet adapter

    Args:
        properties: configuration properties
    Returns:
        run args
    """
    return dict(
	command=f"./seed --duration={properties['runtime']}s",
        volumes={
            properties["bundledir"]: {
                "bind": "/home/mininet/bundle",
                "mode": "ro"
            },
            properties["resultdir"]: {
                "bind": "/home/mininet/result",
                "mode": "rw"
            }
        }
    )
