from .                  import MobilitySolution
from ..utils.pairwise   import pairwise
import pprint

class MapMe(MobilitySolution):
    __name__ = "map-me"

    def get_nodes(self):
        return ['source', 'destination', 'anchor']

    def on_simulate(self):
        # FIB: router -> next_hop
        self.fib = dict()
        
    def after_initial_position(self, nodes, topology):
        # Global routing update. We initialize routers' FIB with shortest paths
        # towards destination. Since the graph is not oriented, we can compute
        # the shortest path from destination towards all other nodes, then
        # populate the FIB accordingly.
        dst = nodes['destination']
        sssp = topology.sssp(dst.get_position())
        for _, path in sssp.items():
            for x, y in pairwise(path):
                self.fib[y] = x 

    def after_move(self, nodes, topology):
        """
        After a producer's movement, we send an IU towards the old position
        """
        dst = nodes['destination']
        
        path = self._get_path(topology, dst.get_position(), dst.get_old_position())
        
        # We update the list of traces (set route backwards)
        for x, y in pairwise(path):
            self.fib[y] = x
        self.fib[dst.get_position()] = None


    def _get_path(self, topology, src, dst):
        # Compute the forwarding path
        cur = src
        path = [src]
        while cur != dst:
            cur = self.fib[cur]
            path.append(cur)
        return path

    def compute_results(self, nodes, topology):
        src    = nodes['source'].get_position()
        dst    = nodes['destination'].get_position()

        path = self._get_path(topology, src, dst)

        return {
            'path': None, #path,
            'path_len': float(len(path) - 1),
            'sp_len': topology.d(src, dst)
        }
