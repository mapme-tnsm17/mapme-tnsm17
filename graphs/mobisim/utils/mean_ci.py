# http://stackoverflow.com/questions/15033511/compute-a-confidence-interval-from-sample-data

DEFAULT_CONFIDENCE = 0.95

import numpy as np
import scipy as sp
import scipy.stats

def mean_confidence_interval(data, confidence = DEFAULT_CONFIDENCE):
    a = 1.0 * np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return m, h # m-h, m+h

