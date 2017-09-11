import os

from statistics     import Statistics
from node           import Node
from patterns       import MAX_MOVEMENT, LOG_MOVEMENTS
from task           import Task

RUNS = 1000

RESULT_PATH = "results"

class MonteCarloSimulation(Task):
    def __init__(self, topology, pattern, solution):
        self._topology = topology
        self._pattern = pattern
        self._solution = solution

        self._results = list()

    def is_done(self):
        fn = os.path.join(RESULT_PATH, self.get_name())
        return os.path.exists(fn) and os.stat(fn).st_size != 0

    def _do_callback(self, cb_name, *args, **kwargs):
        for cls in [self._topology, self._pattern, self._solution]:
            try:
                getattr(cls, cb_name)(*args, **kwargs)
            except Exception, e:
                import traceback
                traceback.print_exc()
                print "E:", e

    def get_name(self):
        # Write results
        t_name = self._topology.get_name()
        mp_name = self._pattern.get_name()
        ms_name = self._solution.get_name()
        
        return "%(t_name)s_%(mp_name)s_%(ms_name)s" % locals()

    def run(self):
        # Callback: on_init
        print "SIMULATING", self.get_name()
        self._do_callback('on_init')

        fn = os.path.join(RESULT_PATH, self.get_name())
        stats = Statistics(self._topology, self._pattern, self._solution, fn)

        for _ in range(0, RUNS):
            stats.new_run()

            nodes = dict()
            for node in self._solution.get_nodes():
                perimeter = node if self._topology.has_perimeter(node) else None
                nodes[node] = Node(moving=(node=="destination"), perimeter=perimeter)
            self._do_callback('on_simulate')

            # Setup node initial position
            self._pattern.move_nodes(nodes, self._topology)
            self._do_callback('after_initial_position', nodes, self._topology)
            result = self._solution.compute_results(nodes, self._topology)
            stats.add_result(0, result)

            for movement in range(1, MAX_MOVEMENT+1):
                self._do_callback('before_move', nodes, self._topology)
                self._pattern.move_nodes(nodes, self._topology)
                self._do_callback('after_move', nodes, self._topology)

                result = self._solution.compute_results(nodes, self._topology)
                stats.add_result(movement, result)

        stats.done()

