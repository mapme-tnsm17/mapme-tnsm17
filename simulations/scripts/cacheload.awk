#Input: cs-trace.txt
#Input format: Time Node Type Packets
#Output:
#Output format:

function reset_stats()
{
    hits = 0
    total = 0
}

function dump_stats()
{
    print prev_time, total, hits
}

BEGIN {
    prev_time = 0
    interval = 10
}

{
    # Keep prefix_dump only
    if ($3 == "CacheHitsUp" || $3 == "CacheMissesUp")
        next

    if ($1 - prev_time >= interval) {
        dump_stats()
        reset_stats()
        prev_time = $1
    }

    if ($3 == "CacheHitsDown")
        hits += $4
    total += $4
}

END {
    dump_stats()
}
