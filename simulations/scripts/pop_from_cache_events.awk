#!/usr/bin/awk -f 
# Input: zipf cache-events
# Input format: TIME HITMISS NAME
# Output:
# Output format: OBJECT:1 EMPIRICAL_PROB:2 PROB:3 COUNT:4 HITS:5 HIT_RATE:6 COUNT_ALL:7 EMPIRICAL_PROB_ALL:8 HITS_ALL:9 HIT_RATE_ALL:10
# NEW:
# We compute the empirical popularity of content chunks with
# sequence number 0. The former code was per-chunk popularity, this
# is per-content popularity.

@include "scripts/helpers/split_name.awk"

# Extract zipf probability from first file
FNR==NR {
  popularity[$1] = $2
  next
}

{
    split_name($3, result)

    # Ignore requests to "prefix_down"
    #if (result["pfx"] != "prefix")
    #    next

    #print "***", $2, $3, result["pfx"], result["obj"], result["seq"]

    count_all[result["obj"]]++
    if ($2 == "HIT") {
      hit_all[result["obj"]]++
    }
    total_all++

    if (result["seq"] == 0) {
        count[result["obj"]]++
        if ($2 == "HIT") {
          hit[result["obj"]]++
        }
        total++
    }
}

END {
    for (obj in count) {
        print obj, count[obj] / total, popularity[obj], count[obj], hit[obj]?hit[obj]:0, hit[obj] / count[obj], count_all[obj], count_all[obj] / total_all, hit_all[obj]?hit_all[obj]:0, hit_all[obj] / count_all[obj]
    }
}
