#!/usr/bin/awk -f 

# Input: cache-event-c1.out
# Input format: TIME HIT/MISS NAME
# Output format: TIME TOTAL_LOAD CACHE_LOAD
#
# Note: overages over <avg_slots> intervals

@include "scripts/in_path_caching/config.awk"
@include "scripts/in_path_caching/helpers/split_name.awk"

function print_load()
{
    print old_time * avg_slots, 1.0 * chunks * chunk_size / capacity / avg_slots, 1.0 * chunks_cache * chunk_size / capacity / avg_slots
}

BEGIN {
  avg_slots = 100
}

{
    split_name($3, result)
    if (result["pfx"] == "prefix")
        next;
    time = int($1 / avg_slots)
    if (time != old_time) {
        print_load()
        chunks = 0
        chunks_cache = 0
    }
    if ($2 == "HIT")
      chunks_cache += 1
    chunks += 1
    old_time = time

}

END {
  print_load()
}
