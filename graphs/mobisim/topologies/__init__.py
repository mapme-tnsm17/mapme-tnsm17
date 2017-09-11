import os, sys, itertools, fnss, networkx as nx
from types import StringTypes

ROCKETFUEL_PATH = os.path.expanduser("data/rocketfuel")

# XXX inherit from there for Rocketfuel, etc.
# We make a MonteCarlo simulation associating a Topology and a MobilityModel
class Topology(object):

    # XXX Don't run if output file already exists

    def __init__(self, name):
        # name is a list of stringlets
        self._name = name.split('-') if isinstance(name, StringTypes) else name
        self._topology = None
        self._all_sp = dict()

    def sp(self, x, y):
        if x not in self._all_sp:
            self._all_sp[x] = nx.single_source_shortest_path(self._topology, x)
        return self._all_sp[x][y]

    def sssp(self, x):
        if x not in self._all_sp:
            self._all_sp[x] = nx.single_source_shortest_path(self._topology, x)
        return self._all_sp[x] 

    def d(self, x, y):
        return float(len(self.sp(x, y)) - 1)

    def get_name(self):
        return "-".join(self._name)

    # Callbacks
    def NA(*args, **kwargs): pass
    on_init = NA
    on_simulate = NA
    before_move = NA
    after_move = NA
    after_initial_position = NA

    def on_init(self):
        # XXX
        # XXX For some topologies, we need multiple iterations. eg E-R
        # XXX
        
        # Reuse topology information
        if self._topology:
            return

        self._load_topology()

        old_nodes = self._topology.number_of_nodes()
        old_links = self._topology.number_of_edges()

        # Keeping the largest connected component
        max = 0
        for component in nx.connected_component_subgraphs(self._topology):
            l = len(component.nodes())
            if l > max:
                max = l
                self._topology = component

        #self._topology = nx.minimum_spanning_tree(self._topology)

        num_nodes = self._topology.number_of_nodes()
        num_links = self._topology.number_of_edges()
        
        name = '-'.join(self._name)
        print >>sys.stderr, "Topology: %(name)s: %(num_nodes)s (%(old_nodes)s) nodes, %(num_links)s (%(old_links)s)" % locals()

    def _load_rocketfuel_topology(self):
        # name: rocketfuel-as_name-heuristic-...
        as_name, heuristic = self._name[1:3]
        filename = '%s/maps/%s.%s' % (ROCKETFUEL_PATH, as_name, heuristic)
        self._topology = fnss.parse_rocketfuel_isp_map(filename).to_undirected()

    def _load_fat_tree(self):
        self._topology = fnss.topologies.datacenter.fat_tree_topology(4)

        # Perimeter
        for n, d in self._topology.nodes_iter(data=True):
            if d['layer'] == 'leaf':
                d['perimeter'] = ['destination']
            elif d['layer'] == 'core':
                d['perimeter'] = ['anchor', 'source']

        print "Writing dot file"
        nx.write_dot(self._topology, '/tmp/out.dot')

    def _connect_backbone(self):
        # Connect backbone nodes = nodes with degree > 3
        deg = self._topology.degree()
        clients = set([n for n in deg if deg[n] <= 3])
        clients_gw = set()
        for client in clients:
            clients_gw |= set(self._topology.neighbors(n))
        all_nodes = set(self._topology.nodes())
        backbone_nodes = all_nodes - clients_gw

        for pair in itertools.combinations(backbone_nodes, 2):
            if not self._topology.has_edge(*pair):
                self._topology.add_edge(*pair)

    def _load_topology(self):
        print "TOPO", self._name[0]
        if self._name[0] in ['rocketfuel', 'rocketfuelbb']:
            self._load_rocketfuel_topology()
        elif self._name[0] == 'fattree':
            self._load_fat_tree()
        else:
            raise NotImplemented

        if self._name[0] == 'rocketfuelbb':
            self._connect_backbone()

    def has_perimeter(self, name):
        if self._name[0] in ['rocketfuel', 'rocketfuelbb']:
            return False
        elif self._name[0] == 'fattree':
            return True
        else:
            return False

    def get_nodes(self, perimeter):
        if not perimeter:
            return self._topology.nodes()
        else:
            return [n for n, d in self._topology.nodes_iter(data=True) if perimeter in d.get('perimeter', list())]

    def get_neighbors(self, node):
        return self._topology.neighbors(node)

#if heuristic != "r1.cch": # We keep only one
#    continue
## only for r1.cch ?
#latencies_fn = "%s/weights/%s/latencies.intra" % (path, as_name)
#weigths_fn   = "%s/weights/%s/weights.intra" % (path, as_name)
#if not os.path.exists(latencies_fn) or not os.path.exists(weigths_fn):
#    continue
#topology = fnss.parse_rocketfuel_isp_latency(latencies_fn, weigths_fn).to_undirected()

#DEPRECATED|    def compute_metrics(self):
#DEPRECATED|        name = self._mobility_pattern + '-'.join(self._name)
#DEPRECATED|        with open(os.path.join(RESULT_PATH, name), 'w') as f:
#DEPRECATED|            print >>f, "name metric method movement mean ci"            # XXX
#DEPRECATED|            for method in MAP_METHODS.keys():
#DEPRECATED|                cls = MAP_METHODS[method]
#DEPRECATED|                all_results = list()
#DEPRECATED|                while len(all_results) < RUNS: # MONTE CARLO SIMULATION HERE XXX
#DEPRECATED|                    results = cls(self._topology).run()
#DEPRECATED|                    all_results.append(results)
#DEPRECATED|
#DEPRECATED|                # Write file part                                       # XXX
#DEPRECATED|                for metric in METRICS:
#DEPRECATED|                    for movement in LOG_MOVEMENTS:
#DEPRECATED|                        # Get the list of metrics for a given movement
#DEPRECATED|                        metric_values = [result[metric][movement] for result in all_results]
#DEPRECATED|                        mean, ci = mean_confidence_interval(metric_values)
#DEPRECATED|                        print >>f, name, metric, method, movement, mean, ci
#DEPRECATED|


# ROCKETFUEL NOTES

# asn.cch is the raw cch file, containing all links for nodes that could fall
# within the ISP.  This may include a chain of routers leading into an unnamed
# customer's address space.
# 
# asn.r0.cch is the opposite, and includes only routers that we believe are part
# of the ISP because they're named like backbone and gateway routers (or at
# least as best as we can recover).
# 
# asn.r1.cch is in between, including a "fringe" of radius 1 around the backbone
# and gateway routers.  This is the dataset used primarily in the Rocketfuel
# paper because we expect it to be robust to problems in recovering the ISP's
# naming convention and it shows the most structure we're interested in, without
# router chains leading to customers.


