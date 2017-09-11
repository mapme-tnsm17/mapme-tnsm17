#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, fnss
import networkx as nx

def main(filename):
    topology = fnss.parse_rocketfuel_isp_map(filename).to_undirected()


    old_nodes = topology.number_of_nodes()
    old_links = topology.number_of_edges()
    
    # Keeping the largest connected component
    max = 0
    for component in nx.connected_component_subgraphs(topology):
        l = len(component.nodes())
        if l > max:
            max = l
            topology = component

    num_nodes = topology.number_of_nodes()
    num_links = topology.number_of_edges()

    print "%(filename)s %(num_nodes)s (%(old_nodes)s) & %(num_links)s (%(old_links)s) links" % locals()

if __name__ == '__main__':
    filename = sys.argv[1]
    main(filename)
