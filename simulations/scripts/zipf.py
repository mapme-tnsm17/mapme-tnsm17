#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

def zipf(alpha, catalog_size):
    pi = [0] * (catalog_size+1)
    c = 1
    s = 0
    for i in range(1, catalog_size+1):
        pi[i] = c / i**alpha
        s += pi[i]
    for i in range(1, catalog_size+1):
        print i, pi[i]/s

    # 1/n^a    
if __name__ == '__main__':
    zipf(float(sys.argv[1]), int(sys.argv[2]))
