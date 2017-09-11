from . import MobilityPattern

# XXX First movement is wrong


class WaypointMobility(MobilityPattern):

    __name__ = "waypoint"

    def on_simulate(self, *args, **kwargs):
        # Reset internal state
        # - Followed path
        self.path = None
        # - Position on path
        self.path_pos = 0

    def move_nodes(self, nodes, topology):
        # What if the path goes through an existing node position ? which is not
        # allowed in random...

        for name, node in nodes.items():
            # Static nodes: set initial position
            if not node.get_position():
                node.set_position(self.choose_available_position(name, nodes, topology))
                continue
            if not node.is_moving():
                continue
                
            # Eventually, choose a new waypoint to aim at
            if not self.path:
                waypoint = self.choose_available_position(name, nodes, topology)
                self.path = topology.sp(node.get_position(), waypoint)
                if self.path and len(self.path) < 2:
                    import sys
                    sys.exit(1)
                self.path_pos = 1
                #print "New waypoint", self.path
            
            # Move to next position towards waypoint
            node.set_position(self.path[self.path_pos])

            self.path_pos += 1
            if self.path_pos >= len(self.path):
                # Waypoint reached
                self.path = None
                node.set_think_time(True)
            else:
                node.set_think_time(False)
