from ..utils.plugin_factory import PluginFactory

class MobilitySolution(object):

    __metaclass__ = PluginFactory

    _registry = dict()

    # Callbacks
    def NA(*args, **kwargs): pass
    on_init = NA
    on_simulate = NA
    before_move = NA
    after_move = NA
    after_initial_position = NA

    def get_name(self):
        return self.__name__

from anchor import Anchor
from kite import Kite
from map_me import MapMe
#from map_me_in import MapMeIN
#from ours import Ours
#from ours_in import OursIN
