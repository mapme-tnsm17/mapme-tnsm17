from .                  import MobilitySolution
from ..utils.pairwise   import pairwise

class Ours(MobilitySolution):
    __name__ = "ours"

    def get_nodes(self):
        return ['source', 'destination', 'anchor']

    def on_simulate(self):
        # Clear state
        self.dst0 = None
        self.next_hops = dict()

    def after_move(self, nodes, topology):
        """
        After a producer's movement, we send an IU towards the old position
        """
        dst = nodes['destination']
        
        if not dst.get_old_position():
            # Position set up : we just remember the initial position, since we
            # assume initial routing = shortest path to initial position.
            self.dst0 = dst.get_position()
            return
            
        path = topology.sp(dst.get_old_position(), dst.get_position())
        
        # We update the list of traces
        for router, next_hop in pairwise(path):
            self.next_hops[router] = next_hop

    def _get_path(self, topology, src, dst):
        # Compute the forwarding path
        path = list()
        
        # Assuming the updates only affect a small part of the network, we
        # avoid building the map for every router, instead we assume they
        # point to the initial position, and only store updates in
        # next_hop[].
        src_dst0 = topology.sp(src, self.dst0)
        for router in src_dst0:
            if router in self.next_hops:
                cur = router
                while cur != dst:
                    path.append(cur)
                    cur = self.next_hops[cur]
                path.append(dst)
                break
            path.append(router)

        return path

    def compute_results(self, nodes, topology):
        src    = nodes['source'].get_position()
        dst    = nodes['destination'].get_position()

        # First time initialization of destination
        if not self.dst0:
            self.dst0 = dst

        path = self._get_path(topology, src, dst)

        return {
            'path_len': float(len(path) - 1),
            'sp_len': topology.d(src, dst)
        }
