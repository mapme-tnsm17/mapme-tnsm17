#!/usr/bin/awk -f 

FNR==1 { 
    match(FILENAME, "r([^-]*)-K([^-]*)-C([^-]*)-s([^-]*)-a([^-]*).*-d([^/]*)/", arr)
    rho = arr[1]
    catalog_size = arr[2]
    cs_fraction = arr[3]
    sigma = arr[4]
    alpha = arr[5]
    duration = arr[6]

    capacity = 10000000
    chunk_size = 8192
}

