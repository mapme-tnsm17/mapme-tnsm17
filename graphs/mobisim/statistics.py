import os
from patterns import LOG_MOVEMENTS
from utils.mean_ci  import mean_confidence_interval
from utils.pairwise import pairwise

METRICS = ['path_len', 'sp_len', 'stretch']

class Statistics(object):
    def __init__(self, topology, pattern, solution, filename):
        self._topology = topology
        self._pattern = pattern
        self._solution = solution
        self._filename = filename

        self._current_run = 0
        self._current_metric_list_dict = None
        self._all_metric_list_dict = list()
        
        return
        fn_paths = "%s-routes" % (filename,)
        self._f_paths = open(fn_paths, 'w') 
        print >>self._f_paths, "run movement from_id to_id"

    def new_run(self):
        self.process_run()

        self._current_run += 1
        self._current_metric_list_dict = dict()
        for metric in METRICS:
            self._current_metric_list_dict[metric] = list()

    def add_result(self, movement, result):
        if result['sp_len'] == 0:
            result['stretch'] = 1
        else:
            result['stretch'] = result['path_len'] / result['sp_len']
        for metric in METRICS:
            self._current_metric_list_dict[metric].append(result[metric])

        # Dump path
        if not result.get('path'):
            return
        for x, y in pairwise(result['path']):
            print >>self._f_paths, self._current_run, movement, x, y


    def process_run(self):
        if self._current_metric_list_dict:
            self._all_metric_list_dict.append(self._current_metric_list_dict)

    def done(self):
        self.process_run()

        # Write results
        t_name = self._topology.get_name()
        mp_name = self._pattern.get_name()
        ms_name = self._solution.get_name()
        
        with open(self._filename, 'w') as f:
            print >>f, "topology pattern solution movement metric mean ci"
            # Write file part : we assume the structure of results is choosen
            # for now
            for metric in METRICS:
                for movement in LOG_MOVEMENTS:
                    # Get the list of metrics for a given movement
                    metric_values = [result[metric][movement] for result in self._all_metric_list_dict]
                    mean, ci = mean_confidence_interval(metric_values)
                    print >>f, t_name, mp_name, ms_name, movement, metric, mean, ci

        return
        self._f_paths.close()
