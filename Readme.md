# Shared Evaluation Environment for software Defined network applications

SEED is a **S**hared **E**valuation **E**nvironment for Software-**D**efined networking applications. SEED simplifies the process of evaluating SDN-applications by

- making SDN-applications (specifically the underlying controller platforms), simulators and scenarios interchangeable
- providing an experiment description with which these components are declared unambiguously
- providing a scenario description format which contains simulator agnostic information on topology and traffic.
- providing a unified frontend for all evaluations

By pooling these components, researchers can spend more of their time on the actual evaluation instead of having to study a specific simulator or creating representative scenarios (which are both highly error-prone tasks).

## Installation

SEED depends on [git](https://www.git-scm.com), [git-lfs](https://git-lfs.github.com/), [python 3](https://www.python.org) and [docker](https://www.docker.com) which need to be installed beforehand. Installation of SEED is performed with three simple commands:

```
$ git clone --recursive https://github.com/KIT-Telematics/seed.git
$ cd seed
$ python3 setup.py install
```

## Quickstart

After installing SEED, you can test SEED by executing this command:

```
$ seed --config -vv configs/config.mininet.datacenter.ryu.pbce.yml
```

It should print the setup and output log of the simulator and controllers.

# Documentation

A comprehensive manual is available [here](documentation/manual.pdf). It provides documentation on the seed command, the experiment and scenario description as well as controller integration.

## Citation

If you used seed for your research, please consider citing our paper (BibTeX):

```
@inproceedings{mci/Dittebrandt2017,
author = {Dittebrandt, Addis and König, Michael and Neumeister, Felix},
booktitle = {INFORMATIK 2017},
doi = {10.18420/in2017_246},
editor = {Eibl, Maximilian and Gaedke, Martin},
interhash = {dbd44d8ad4de998f5e90073cf796a152},
intrahash = {a17d2d5007a0e7541e6994d28fa90d43},
isbn = {978-3-88579-669-5},
pages = { 2413-2423 },
publisher = {Gesellschaft für Informatik, Bonn},
title = {Towards a Shared Evaluation Environment for Software-defined Networking},
url = {https://dl.gi.de/handle/20.500.12116/4021},
year = 2017
}
```
