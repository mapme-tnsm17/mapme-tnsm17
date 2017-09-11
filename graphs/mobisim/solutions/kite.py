from . import MobilitySolution

class Kite(MobilitySolution):
    __name__ = "kite"

    def get_nodes(self):
        return ['source', 'destination', 'anchor']

    def _get_path(self, topology, src, dst, anchor):
        path = topology.sp(src, anchor)
        path.extend(topology.sp(anchor, dst))
        return path

    def compute_results(self, nodes, topology):
        src    = nodes['source'].get_position()
        dst    = nodes['destination'].get_position()
        anchor = nodes['anchor'].get_position()

        anchor_src = topology.sp(anchor, src)
        anchor_dst = topology.sp(anchor, dst)

        # Compute common part in paths
        pos = 0
        max_pos = min(len(anchor_src) - 1, len(anchor_dst) - 1)
        while pos < max_pos:
            if anchor_src[pos] != anchor_dst[pos]:
                break
            pos += 1

        path_len = topology.d(anchor, src) + topology.d(anchor, dst)
        if (pos > 0):
            path_len -= 2 * (pos-1)

        return {
            'path': None, 
            'path_len': path_len,
            'sp_len'  : topology.d(src, dst)
        }
