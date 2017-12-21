"""configurator for pox controller
"""

import os

def configure(properties):
    """configurator function for pox controller

    Args:
        properties: configuration properties
    Returns:
        run args
    """
    volumes = {}
    for application, path in properties["applications"].items():
        volumes[path] = {
            "bind": os.path.join("/usr/src/app/ext", application),
            "mode": "ro"
        }
    return dict(
        command=" ".join(["./pox.py", properties["args"]]),
        volumes=volumes
    )
