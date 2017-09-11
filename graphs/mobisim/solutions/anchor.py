from . import MobilitySolution

class Anchor(MobilitySolution):
    __name__ = "anchor"

    def get_nodes(self):
        return ['source', 'destination', 'anchor']

    def _get_path(self, topology, src, dst, anchor):
        path = topology.sp(src, anchor)[:]
        path.extend(topology.sp(anchor, dst))
        return path

    def compute_results(self, nodes, topology):
        src    = nodes['source'].get_position()
        dst    = nodes['destination'].get_position()
        anchor = nodes['anchor'].get_position()

        path = self._get_path(topology, src, dst, anchor)

        return {
            'path':     None,
            'path_len': topology.d(src, anchor) + topology.d(anchor, dst),
            'sp_len'  : topology.d(src, dst)
        }
