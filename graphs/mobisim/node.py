class Node(object):
    def __init__(self, moving = False, perimeter = None):
        # Boolean flag indicating whether the node is moving
        self._moving = moving
        self._think_time = True
        # List of vertices from the topology on which the node is allowed
        self._perimeter = perimeter

        # Current position
        self._position = None
        self._old_position = None

    def is_moving(self):
        return self._moving

    def is_think_time(self):
        return self._think_time

    def set_think_time(self, flag):
        self._think_time = flag

    def get_position(self):
        return self._position

    def set_position(self, new_position):
        assert new_position is not None
        self._old_position = self._position
        self._position = new_position

    def get_old_position(self):
        return self._old_position

    def get_perimeter(self):
        return self._perimeter if self._perimeter else None
