function split_name(name, result) {
    split(name, tmp, "/")
    result["pfx"] = tmp[2]
    result["obj"] = substr(tmp[3], 7, length(tmp[3]) - 6)

    hex = substr(tmp[4], 5, length(tmp[4]) - 4)
    result["seq"] = strtonum( "0x"substr(tmp[4], 5, length(tmp[4]) - 4) )
}

