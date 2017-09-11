import random

from ..utils.plugin_factory import PluginFactory

MAX_MOVEMENT = 1000
LOG_MOVEMENTS = range(0, MAX_MOVEMENT+1) #[0, 1, 2, 5, 10, 100, 1000]

class MobilityPattern(object):

    __metaclass__ = PluginFactory

    # Callbacks
    def NA(*args, **kwargs): pass
    on_init = NA
    on_simulate = NA
    before_move = NA
    after_move = NA
    after_initial_position = NA

    def get_name(self):
        return self.__name__

    def get_available_position(self, node_name, nodes, topology):
        node = nodes[node_name]
        available = set(topology.get_nodes(node.get_perimeter()))

        #other_positions = set([other_node.get_position() for other_name, other_node in nodes.items() if other_name != node_name ])
        #available -= other_positions
        all_positions = set([other_node.get_position() for other_name, other_node in nodes.items()])
        available -= all_positions
        return available

    def choose_available_position(self, node_name, nodes, topology):
        available = self.get_available_position(node_name, nodes, topology)
        return random.choice(list(available))

from waypoint import WaypointMobility
from uniform import UniformMobility
#from random_walk import RandomWalkMobility
