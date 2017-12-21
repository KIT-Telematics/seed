"""main module
"""

import argparse
import importlib.util
import os
import shutil
import tempfile
import threading
import uuid

import docker
import yaml

from . import preprocess

def __import_configurator(path):
    conf_path = os.path.join(path, "configurator.py")
    spec = importlib.util.spec_from_file_location("configurator", conf_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def __start_container(client, base_path, properties, build, nocache, network,
                      alias=None, **container_opts):
    image_path = os.path.join(base_path, properties.pop("image"))

    cache = client.images.list(filters={'label':f"seed={image_path}"})
    if (not build) and cache:
        print(" Using Cached Image")
        image = cache[0]
    else:
        print(" Building Image... ", end='', flush=True)
        image = client.images.build(
            path=image_path,
            nocache=nocache,
            rm=True,
            pull=True,
            labels={'seed':image_path}
        )
        print("Done")

    configurator = __import_configurator(image_path)
    ret = configurator.configure(properties)
    if isinstance(ret, tuple):
        configurator_opts, teardown = ret
    else:
        configurator_opts, teardown = (ret, lambda: None)
    del configurator

    print(" Starting Container... ", end='', flush=True)
    container = client.containers.create(
        image=image.id,
        detach=True,
        init=True,
        **container_opts,
        **configurator_opts
    )
    print("Done")

    network.connect(
        container,
        aliases=[alias] if alias is not None else None
    )

    container.start()

    return (container, teardown)

def __log_container(name, container):
    logs = container.logs(
        stdout=True,
        stderr=True,
        stream=True,
        follow=True
    )
    for log in logs:
        print(f"{name}:", log.decode(), end='', flush=True)

def __parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--wdir', default="",
        help="path to working directory")
    parser.add_argument('--config', default='config.yml',
        help="path to config file relative to working directory")
    parser.add_argument('--adapterdir', default='adapters',
        help="adapter search path relative to working directory")
    parser.add_argument('--applicationdir', default='applications',
        help="application search path relative to working directory")
    parser.add_argument('--bundledir', default='bundles',
        help="bundle search path relative to working directory")
    parser.add_argument('--controllerdir', default='controllers',
        help="controller search path relative to working directory")
    parser.add_argument('--build', action='store_true',
        help="rebuild adapter or controller images")
    parser.add_argument('--nocache', action='store_true',
        help="don't use build cache on adapter or controller rebuild")
    parser.add_argument('--verbose', '-v', action='count', default=0,
        help="print adapter (1st level) and controller (2nd level) log")
    args = parser.parse_args()
    args.wdir = os.path.join(os.getcwd(), args.wdir)
    return args

def main():
    """main function
    """
    args = __parse_args()

    adapter, controllers, network, bundledir = None, None, None, None
    try:
        with open(os.path.join(args.wdir, args.config)) as config_f:
            config = yaml.load(config_f)["config"]

        run_id = str(uuid.uuid4())
        print(f"Starting With Run Id \"{run_id}\"")

        #Set up directories
        bundledir = tempfile.TemporaryDirectory()
        resultdir = os.path.join("results", run_id)
        os.makedirs(resultdir)

        bundlepath = os.path.join(
            args.wdir,
            args.bundledir,
            config["bundle"]["name"],
            "bundle"
        )
        preprocess.preprocess_bundle(
            bundlepath + ".xml",
            os.path.join(bundledir.name, "bundle.xml"),
            config["bundle"]['parameters']
        )
        shutil.copy(
            bundlepath + ".controller-bindings.yml",
            os.path.join(bundledir.name, "bundle.controller-bindings.yml")
        )

        #Set up network and containers
        client = docker.from_env()
        network = client.networks.create(run_id)

        controllers = {}
        for name, properties in config["controllers"].items():
            print(f"Starting Controller \"{name}\"")
            applications = {
                application: os.path.join(
                    args.wdir,
                    args.applicationdir,
                    application)
                for application in properties['applications']}
            controllers[name] = __start_container(
                client,
                os.path.join(args.wdir, args.controllerdir),
                dict(properties, **{
                    "applications": applications,
                    "bundledir": bundledir.name,
                    "resultdir": os.path.join(os.getcwd(), resultdir)
                }),
                args.build,
                args.nocache,
                network,
                alias=name
            )
            print("Done")

        print(f"Starting Adapter \"{config['adapter']['image']}\"")
        adapter = __start_container(
            client,
            os.path.join(args.wdir, args.adapterdir),
            dict(config["adapter"], **{
                "bundledir": bundledir.name,
                "resultdir": os.path.join(os.getcwd(), resultdir),
                "controllers": config["controllers"]
            }),
            args.build,
            args.nocache,
            network,
            privileged=True
        )
        print("Done")

        try:
            if args.verbose >= 2:
                for name, (container, _) in controllers.items():
                    arg = (name, container)
                    threading.Thread(target=__log_container, args=arg).start()
            if args.verbose >= 1:
                __log_container("adapter", adapter[0])
            import time
            time.sleep(10)
            adapter[0].wait()
        except KeyboardInterrupt:
            pass

    finally:
        print("Tearing Down")
        if adapter is not None:
            adapter[0].stop()
            adapter[0].remove()
            adapter[1]()
        if controllers is not None:
            for controller, teardown in controllers.values():
                controller.stop()
                controller.remove()
                teardown()
        if network is not None:
            network.remove()
        if bundledir is not None:
            bundledir.cleanup()

if __name__ == "__main__":
    main()
