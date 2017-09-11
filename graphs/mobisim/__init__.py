#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# NOTES
#  - install dependencies: 
#      apt-get install python-scipy, python-numpy, python-networkx
#      # pip install fnss # not up to date 
#      git clone https://github.com/fnss/fnss.git 
#      python setup.py install
#  - use of to_undirected()
#  - release topology memory when done

import os, sys, glob, multiprocessing

from benchmark  import Benchmark
from patterns   import MobilityPattern
from task       import Task
from topologies import Topology
from utils      import pickle_method

PARALLEL = True
NUM_PROCESSES = multiprocessing.cpu_count()

# <BEGIN: move in TOPOLOGY>
ROCKETFUEL_PATH = os.path.expanduser("data/rocketfuel")

def split_name(name, max_split):
    tokens = name.split('-', max_split)
    if len(tokens) < max_split:
        tokens += (max_split - len(tokens)) * [None]
    return tokens[:max_split]

def get_names(fullname):
    names = list()

    name, as_name, heuristic = split_name(fullname, 3)
    if name in ['rocketfuel', 'rocketfuelbb']:

        if as_name == '*':
            as_names = map(lambda x: os.path.splitext(x)[0], map(os.path.basename, glob.glob("%s/maps/[0-9]*.al" % ROCKETFUEL_PATH)))
        else:
            as_names = [as_name]

        if heuristic == '*':
            heuristics = ['r0.cch', 'r1.cch', 'cch']
        else:
            heuristics = [heuristic]


        for as_name in as_names:
            for heuristic in heuristics:
                names.append("%s-%s-%s" % (name, as_name, heuristic))
    else:
        names.append("%s-%s" % (name, as_name))

    return names
# <END: move in TOPOLOGY>

def main():
    # XXX Deduce benchmark scope from commandline arguments
    # Topology names
    if len(sys.argv) == 2:
        names = sys.argv[1:]
    else:
        names = get_names('rocketfuel-*-r0.cch')
        #names.extend(get_names('rocketfuelbb-*-r0.cch'))

    topologies = [Topology(name) for name in names]
    patterns = [p() for p in MobilityPattern.factory_list().values()]

    tasks = list()
    for topology in topologies:
        for pattern in patterns:
            tasks.extend(Benchmark(topology, pattern).get_tasks())

    if PARALLEL:
        pool = multiprocessing.Pool(processes=NUM_PROCESSES)
        pool.map(Task.virtual_run, tasks)
        pool.close()
        pool.join()
    else:
        for task in tasks:
            task.run()
