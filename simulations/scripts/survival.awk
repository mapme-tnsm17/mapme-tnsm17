# Input: zipf || a list of cache events
# Input format: RANK POP || TIME HIT/MISS NAME
# Output: ITEM SURVIVAL
#
# - At what level do we compute survival: chunk / content ?
# - How to define survival at the content level ?

@include "scripts/config.awk"
@include "scripts/helpers/split_name.awk"

# Extract zipf probability from first file
FNR==NR {
  popularity[$1] = $2
  next
}

# First line of second file


{
    # Compute survival at chunk level
    item = $3

    # Ignore before 500s & MISS events & prefix
    if ($1 < stop_time || $2 == "MISS")
        next
    split_name($3, result)
    if (result["pfx"] == "prefix")
        next

    # 0 = No hit after stop_time
    max_hit[item] = 0

    # For each file, we want the last hit
    if ($2 == "HIT") {
        if (!max_hit[item] || $1 > max_hit[item]) {
            max_hit[item] = $1
        }
    }


}

END {
    # Compute survival time
    for (item in max_hit) {
        split_name(item, result)

        if (max_hit[item] == 0) {
            print item, 0
        } else {
            print item, result["obj"], result["seq"], popularity[result["obj"]], max_hit[item] - stop_time
        }
    }
}
