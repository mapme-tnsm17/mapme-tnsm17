#!/usr/bin/env python3

import errno
import math
import os
import tempfile

import networkx as nx
import matplotlib.pyplot as plt

# Paper has always twice as much edges than me

DEFAULT_BW='10Mbps'
DEFAULT_METRIC=1        # or weight u.a.r from [1, 100]
DEFAULT_DELAY='1ms'    # 1/delay ?
DEFAULT_QUEUE=100

#------------------------------------------------------------------------------

def mkdir(directory):
    """
    Create a directory (mkdir -p).
    Args:
        directory: A String containing an absolute path.
    Raises:
        OSError: If the directory cannot be created.
    """
    # http://stackoverflow.com/questions/600268/mkdir-p-functionality-in-python
    try:
        if not os.path.exists(directory):
            print("Creating '%s' directory" % directory)
        os.makedirs(directory)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(directory):
            pass
        else:
            raise OSError("Cannot mkdir %s: %s" % (directory, e))

def check_writable_directory(directory):
    """
    Tests whether a directory is writable.
    Args:
        directory: A String containing an absolute path.
    Raises:
        RuntimeError: If the directory does not exists or isn't writable.
    """
    if not os.path.exists(directory):
        raise RuntimeError("Directory '%s' does not exists" % directory)
    if not os.access(directory, os.W_OK | os.X_OK):
        raise RuntimeError("Directory '%s' is not writable" % directory)
    try:
        with tempfile.TemporaryFile(dir = directory):
            pass
    except Exception as e:
        raise RuntimeError("Cannot write into directory '%s': %s" % (directory, e))

def make_writable_directory(directory):
    """
    See mkdir()
    """
    mkdir(directory)

def ensure_writable_directory(directory):
    """
    Tests whether a directory exists and is writable. If not,
    try to create such a directory.
    Args:
        directory: A String containing an absolute path.
    Raises:
        RuntimeError: If the directory does not exists and cannot be created.
    """
    try:
        check_writable_directory(directory)
    except RuntimeError as e:
        make_writable_directory(directory)

#------------------------------------------------------------------------------
class Graph:
    __expected__ = 0

    def __init__(self, *args, **kwargs):
        self.G = None
        # set by layouts: circular, spring, shell, spectral, (graphviz, pydot)
        self.pos = None 

    def keep_largest_connected_component(self):	
        # Keeping the largest connected component
        if nx.is_directed(self.G):
            return
        max = 0
        for component in nx.connected_component_subgraphs(self.G):
            l = len(component.nodes())
            if l > max:
                max = l
                self.G = component

    def dump_ns3(self, filename):
        # We need to convert tuples to string
        to_str = lambda name : str(name) if isinstance(name, int) else '_'.join(str(x) for x in name)

        with open(filename, 'w') as f:
            print('router', file=f)
            print('#node\tcomment\tyPos\txPos', file=f)
            for node in self.G.nodes_iter():
                x, y = self.pos[node] if self.pos is not None else (0, 0)
                print('{}\t{}\t{}\t{}'.format(to_str(node), 'NA', y, x), file=f)
            print('link', file=f)
            print('#srcNode\tdstNode\tbandwidth\tmetric\tdelay\tqueue', file=f)
            for (src, dst, data) in self.G.edges_iter(data=True):
                print('{}\t{}\t{}\t{}\t{}\t{}'.format(
                        to_str(src),
                        to_str(dst),
                        DEFAULT_BW,
                        DEFAULT_METRIC,
                        DEFAULT_DELAY,
                        DEFAULT_QUEUE), file=f)

#------------------------------------------------------------------------------

class Cycle(Graph):
    __type__ = 'cycle'
    __expected__ = (30, 60)

    DEFAULT_N = 30

    def __init__(self, n = DEFAULT_N, seed = None):
        self.G = nx.cycle_graph(n)
        self.pos = nx.circular_layout(self.G, scale = 10)

class Grid2D(Graph):
    __type__ = 'grid-2d'
    __expected__ = (100, 360)

    DEFAULT_M = 10
    DEFAULT_N = 10

    def __init__(self, m = DEFAULT_M, n = DEFAULT_N, seed = None):
        self.G = nx.grid_2d_graph(m, n)
        self.pos = None

class Hypercube(Graph):
    __type__ = 'hypercube'
    __expected__ = (128, 896)

    DEFAULT_N = 7

    def __init__(self, n = DEFAULT_N, seed = None):
        self.G = nx.hypercube_graph(n)
        self.pos = None

class Expander(Graph):
    __type__ = 'expander'
    __expected__ = (100, 716)

    DEFAULT_N = 10

    def __init__(self, n = DEFAULT_N, seed = None):
        self.G = nx.margulis_gabber_galil_graph(n)
        self.pos = None

#class Expander2(Graph):
#    __type__ = 'expander2'
#    __expected__ = (100, 716)
#
#    DEFAULT_P = 100
#
#    def __init__(self, p = DEFAULT_P, seed = None):
#        self.G = nx.chordal_cycle_graph(p)
        self.pos = None

class ER(Graph):
    __type__ = 'erdos-renyi'
    __expected__ = (100, 1042)

    DEFAULT_N = 100
    DEFAULT_P = 0.10

    def __init__(self, n = DEFAULT_N, p = DEFAULT_P, seed = None):
        self.G = nx.erdos_renyi_graph(n, p, seed=seed)
        self.pos = None

class Regular(Graph):
    """
    Returns a random d-regular graph on n nodes.

    The resulting graph has no self-loops or parallel edges.

    Parameters:
    d (int) – The degree of each node.
    n (integer) – The number of nodes. The value of n * d must be even.
    seed (hashable object) – The seed for random number generator.
    """
    __type__ = 'regular'
    __expected__ = (100, 300)

    DEFAULT_D = 3
    DEFAULT_N = 100

    def __init__(self, d = DEFAULT_D, n = DEFAULT_N, seed = None):
        self.G = nx.random_regular_graph(d, n, seed=seed)
        self.pos = nx.spectral_layout(self.G, scale=10)

class WattsStrogatz(Graph):
    """
    Return a Watts–Strogatz small-world graph.

    Parameters:
    n (int) – The number of nodes
    k (int) – Each node is joined with its k nearest neighbors in a ring topology.
    p (float) – The probability of rewiring each edge
    seed (int, optional) – Seed for random number generator (default=None)
    """
    __type__ = 'watts-strogatz'
    __expected__ = (100, 400)

    DEFAULT_N = 100
    DEFAULT_K = 4
    DEFAULT_P = 0.10

    def __init__(self, n = DEFAULT_N, k = DEFAULT_K, p = DEFAULT_P, seed = None):
        self.G = nx.watts_strogatz_graph(n, k, p, seed=seed)
        self.pos = None

#class SmallWorld1(Graph):
#    """
#    Return a Newman–Watts–Strogatz small-world graph.
#
#    Parameters:
#    n (int) – The number of nodes.
#    k (int) – Each node is joined with its k nearest neighbors in a ring
#    topology.
#    p (float) – The probability of adding a new edge for each edge.
#    seed (int, optional) – The seed for the random number generator (the default
#            is None).
#    """
#    __type__ = 'small-world1'
#    __expected__ = (100, 491)
#
#    DEFAULT_N = 100
#    DEFAULT_K = 5
#    DEFAULT_P = 0.10
#
#    def __init__(self, n = DEFAULT_N, k = DEFAULT_K, p = DEFAULT_P, seed = None):
#        self.G = nx.newman_watts_strogatz_graph(n, k, p, seed=seed)
        self.pos = None

class SmallWorld(Graph):
    """
    Return a navigable small-world graph.

    A navigable small-world graph is a directed grid with additional long-range
    connections that are chosen randomly.

    [...] we begin with a set of nodes [...] that are identified with the set of
    lattice points in an ￼ square, ￼, and we define the lattice distance between
    two nodes ￼ and ￼ to be the number of “lattice steps” separating them: ￼.
    For a universal constant ￼, the node ￼ has a directed edge to every other
    node within lattice distance ￼ — these are its local contacts. For universal
    constants ￼ and ￼ we also construct directed edges from ￼ to ￼ other nodes
    (the long-range contacts) using independent random trials; the ￼ has
    endpoint ￼ with probability proportional to ￼.

    —[1]

    Parameters:
    n (int) – The number of nodes.
    p (int) – The diameter of short range connections. Each node is joined with
    every other node within this lattice distance.
    q (int) – The number of long-range connections for each node.
    r (float) – Exponent for decaying probability of connections. The
    probability of connecting to a node at lattice distance ￼ is ￼.
    dim (int) – Dimension of grid
    seed (int, optional) – Seed for random number generator (default=None).
    """
    __type__ = 'small-world'
    __expected__ = (100, 491)

    DEFAULT_N = 10
    DEFAULT_P = 1
    DEFAULT_Q = 1
    DEFAULT_R = 2
    DEFAULT_DIM = 2

    def __init__(self, n = DEFAULT_N, p = DEFAULT_P, q = DEFAULT_Q, r =
            DEFAULT_R, dim = DEFAULT_DIM, seed = None):
        self.G = nx.navigable_small_world_graph(n, p, q, r, dim, seed)
        self.pos = None

class BarabasiAlbert(Graph):
    """
    Returns a random graph according to the Barabási–Albert preferential attachment model.

    A graph of n nodes is grown by attaching new nodes each with m edges that are preferentially attached to existing nodes with high degree.

    Parameters:
    n (int) – Number of nodes
    m (int) – Number of edges to attach from a new node to existing nodes
    seed (int, optional) – Seed for random number generator (default=None).
    """
    __type__ = 'barabasi-albert'
    __expected__ = (100, 768)

    DEFAULT_N = 100
    DEFAULT_M = 4

    def __init__(self, n = DEFAULT_N, m = DEFAULT_M, seed = None):
        self.G = nx.barabasi_albert_graph(n, m)
        self.pos = None

def inheritors(klass):
    subclasses = set()
    work = [klass]
    while work:
        parent = work.pop()
        for child in parent.__subclasses__():
            if child not in subclasses:
                subclasses.add(child)
                work.append(child)
    return subclasses

if __name__ == '__main__':
    SEED = None # 123456789

    ensure_writable_directory('out')


    for cls in sorted(inheritors(Graph), key=lambda x: x.__type__):
        graph = cls(seed = SEED)
        graph.keep_largest_connected_component()
        #nx.draw(graph.G)
        #plt.show()
        if graph.G is None:
            print('{}'.format(type(graph).__type__))
            continue

        status = 'OK' if len(graph.G.nodes()) == graph.__expected__[0] and len(graph.G.edges()) == graph.__expected__[1] / 2 else 'FAIL'
        if graph.__type__ == 'erdos-renyi':
            if len(graph.G.nodes()) == graph.__expected__[0]:
                expected = graph.__expected__[1] / 2
                got = len(graph.G.edges())
                diff = math.fabs((got - expected) * 100/ expected)
                if diff < 10: # Tolerance : 10%
                    status = 'OK (diff={:.2f}%)'.format(diff)
                else:
                    status = 'FAIL (diff={:.2f}%)'.format(diff)

        print('{:20s}|V|={}\t|E|={}\t{}\t\texpected = {}\t{}'.format(
                    graph.__type__,
                    len(graph.G.nodes()),
                    len(graph.G.edges()),
                    'directed' if nx.is_directed(graph.G) else 'non directed',
                    graph.__expected__,
                    status))
        graph.dump_ns3('out/{}.txt'.format(type(graph).__type__))
