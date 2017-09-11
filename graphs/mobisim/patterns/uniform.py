from . import MobilityPattern

class UniformMobility(MobilityPattern):

    __name__ = "uniform"

    def move_nodes(self, nodes, topology):
        old_positions = set([node.get_position() for _, node in nodes.items()])

        for name, node in nodes.items():
            if node.get_position() and not node.is_moving():
                continue

            next_position = self.choose_available_position(name, nodes, topology)
            node.set_position(next_position)
