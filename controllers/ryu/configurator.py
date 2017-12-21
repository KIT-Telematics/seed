"""configurator for ryu controller
"""

import os
import tempfile

def configure(properties):
    """configurator function for ryu controller

    Args:
        properties: configuration properties
    Returns:
        run args
    """

    args = ["ryu-manager", properties["args"]]

    volumes = {
        properties["resultdir"]: {
            "bind": "/result",
            "mode": "rw"
        }
    }
    for application, path in properties["applications"].items():
        volumes[path] = {
            "bind": os.path.join("/usr/src/app/app", application),
            "mode": "ro"
        }

    if "config" in properties:
        temp = tempfile.NamedTemporaryFile(mode="w+", delete=False)
        temp.write(properties["config"])
        temp.close()
        volumes[temp.name] = {
            "bind": "/usr/src/app/ryu.conf",
            "mode": "ro"
        }
        args += ["--config-file", "ryu.conf"]

        def teardown():
            os.remove(temp.name)
    else:
        def teardown():
            pass

    return (dict(
        command=" ".join(args),
        volumes=volumes
    ), teardown)
