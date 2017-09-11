from .                  import Ours
from ..utils.pairwise   import pairwise

class OursIN(Ours):
    __name__ = "ours-in"

    def on_simulate(self):
        Ours.on_simulate(self)
        self.breadcrumbs = dict()
        self.old_waypoint = None # Where to send the IU (hyp: 1 MP)

    def after_move(self, nodes, topology):
        """
        After a producer's movement, we send an IU towards the old position
        """
        dst = nodes['destination']
        dstpos = dst.get_position()

        if not self.dst0:
            # Position set up : we just remember the initial position, since we
            # assume initial routing = shortest path to initial position.
            self.dst0 = dstpos
            return

        if not self.old_waypoint:
            self.old_waypoint = dstpos

        if dst.is_think_time():
            # Send an interest update
            path = topology.sp(topology, self.old_waypoint, dstpos)
            
            # We update the list of traces
            for router, next_hop in pairwise(path):
                self.next_hops[router] = next_hop

            self.old_waypoint = dstpos
            self.breadcrumbs.clear()
        else:
            # IN in the old position allows to reach us at the new position
            # (we don't need to simulate flooding, unless we want to measure the
            # number of messages)
            self.breadcrumbs[dst.get_old_position()] = dstpos


    def get_path(self, topology, src, dst):
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

                    # Follow breadcrumbs if we cannot reach the dst with routing
                    if cur in self.breadcrumbs: # not cur in self.next_hops:
                        while cur != dst:
                            path.append(cur)
                            cur = self.breadcrumbs[cur]
                        break

                    path.append(cur)
                    cur = self.next_hops[cur]

                path.append(dst)
                break
            path.append(router)

        return path
