from .                  import MapMe
from ..utils.pairwise   import pairwise

class MapMeIN(MapMe):
    __name__ = "map-me-in"

    def on_simulate(self):
        MapMe.on_simulate(self)
        self.breadcrumbs = dict()
        self.old_waypoint = None # Where to send the IU (hyp: 1 MP)

    def after_initial_position(self, nodes, topology):
        MapMe.after_initial_position(self, nodes, topology)
        self.old_waypoint = nodes['destination'].get_position()

    def after_move(self, nodes, topology):
        """
        After a producer's movement, we send an IU towards the old position
        """
        dst = nodes['destination']
        dstpos = dst.get_position()

        #import pdb; pdb.set_trace()
        if dst.is_think_time():
            # Send an interest update
            path = self._get_path(topology, dstpos, self.old_waypoint, False)
                
            # We update the list of traces (set route backwards)
            for x, y in pairwise(path):
                self.fib[y] = x
            self.fib[dst.get_position()] = None

            self.old_waypoint = dstpos
            self.breadcrumbs.clear()
        else:
            # IN in the old position allows to reach us at the new position
            # (we don't need to simulate flooding, unless we want to measure the
            # number of messages)
            self.breadcrumbs[dst.get_old_position()] = dstpos

    def _get_in_path(self, cur, dst):
        path = list()
        while cur != dst:
            cur = self.breadcrumbs[cur]
            path.append(cur)
        return path

    def _get_path(self, topology, src, dst, use_breadcrumbs = True):
        # Compute the forwarding path
        cur = src
        path = [src]
        while cur != dst:
            if use_breadcrumbs and cur in self.breadcrumbs:
                path.extend(self._get_in_path(cur, dst))
                return path
            cur = self.fib[cur]
            path.append(cur)
        return path
