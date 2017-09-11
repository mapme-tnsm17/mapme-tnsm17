from . import MobilityPattern
import random

class RandomWalkMobility(MobilityPattern):

    __name__ = "random_walk"

    def move_nodes(self, nodes, topology):
        old_positions = set([node.get_position() for _, node in nodes.items()])

        for name, node in nodes.items():
            if node.get_position() and not node.is_moving():
                continue

            # Change node perimeter ?
            available = self.get_available_position(name, nodes, topology)

            # Unless we are at the first choice of position, constraint the
            # movement to adjacent nodes
            if node.get_position():
                available &= set(topology.get_neighbors(node.get_position()))

            if not available:
                raise Exception, "No available next hop"
            next_position = random.choice(list(available))
            node.set_position(next_position)
