#!/usr/bin/awk -f 

# Input: cs-trace.txt
# Input format: Time Node Type Packets
# Output:

@include "scripts/helpers/params_from_filename.awk"

FNR==1 {
    avg_slots = duration/1000
    if (avg_slots < 1)
        avg_slots = 1
}

function print_load()
{
    print old_time * avg_slots, \
        1.0 * chunks * chunk_size / capacity / avg_slots, \
        1.0 * chunks_cache * chunk_size / capacity / avg_slots, \
        chunks == 0 ? 0 : chunks_cache / chunks
}


{
    time = int($1 / avg_slots)
    if (time != old_time) {
        print_load()
        chunks = 0
        chunks_cache = 0
    }

    # Load = hits+misses reaching C1, C2 and C3 for down prefix
    if (($2 == "C1" || $2 == "C2" || $2 == "C3") && ($3 == "CacheHitsDown" || $3 == "CacheMissesDown")) {
        chunks += $4
        if ($1 > stop_time * 2) {
            after_chunks += $4
        }
        if ($1 > stop_time / 2 && $1 < stop_time) {
            before_chunks += $4
        }
    }

    # Load served by caches = Hits
    if ($3 == "CacheHitsDown") {
        chunks_cache += $4

        if ($1 > stop_time / 2 && $1 < stop_time) {
            if ($2 == "C1" || $2 == "C2" || $2 == "C3") {
                before_chunks_cache_c += $4
            }
            if ($2 == "B1" || $2 == "B2") {
                before_chunks_cache_b += $4
            }
            if ($2 == "A1") {
                before_chunks_cache_a += $4
            } 
            before_chunks_cache += $4

        }

        if ($1 > stop_time * 2) {
            if ($2 == "C1" || $2 == "C2" || $2 == "C3") {
                after_chunks_cache_c += $4
            }
            if ($2 == "B1" || $2 == "B2") {
                after_chunks_cache_b += $4
            }
            if ($2 == "A1") {
                after_chunks_cache_a += $4
            } 
            after_chunks_cache += $4
        }
    }

    # We also keep the average fraction of load served by caches way after the
    # producer has disappeared

    old_time = time
}

END {
    print_load()

    print before_chunks       == 0 ? 0 : before_chunks_cache / before_chunks,         \
          before_chunks_cache == 0 ? 0 : before_chunks_cache_c / before_chunks_cache, \
          before_chunks_cache == 0 ? 0 : before_chunks_cache_b / before_chunks_cache, \
          before_chunks_cache == 0 ? 0 : before_chunks_cache_a / before_chunks_cache > "/dev/stderr" 
    print after_chunks        == 0 ? 0 : after_chunks_cache / after_chunks,           \
          after_chunks_cache  == 0 ? 0 : after_chunks_cache_c / after_chunks_cache,
          after_chunks_cache  == 0 ? 0 : after_chunks_cache_b / after_chunks_cache,
          after_chunks_cache  == 0 ? 0 : after_chunks_cache_a / after_chunks_cache > "/dev/stderr" 
}
