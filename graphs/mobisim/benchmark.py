from solutions import MobilitySolution
from montecarlo import MonteCarloSimulation

class Benchmark(object):
    def __init__(self, topology, mobility_pattern):
        self._topology = topology
        self._mobility_pattern = mobility_pattern

    def get_tasks(self):
        """
        Run a benchmark for each mobility solution.
        """
        tasks = list()
        for name, solution in MobilitySolution.factory_list().items():
            mc = MonteCarloSimulation(self._topology, self._mobility_pattern, solution())
            if not mc.is_done():
                tasks.append(mc)
        return tasks

