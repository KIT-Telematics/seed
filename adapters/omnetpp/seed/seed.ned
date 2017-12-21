package seed;

import inet.networklayer.autorouting.ipv4.*;

simple SeedSimple
{
    parameters:
        string bundle_path;
}

network Seed
{
    submodules:
        seedSimple: SeedSimple;
        configurator: IPv4NetworkConfigurator;
}
