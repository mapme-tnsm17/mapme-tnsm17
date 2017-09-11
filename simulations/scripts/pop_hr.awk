{
    tot++;
    num[$$3]++;
    if ($$2 == "HIT") {
        hits[$$3]++
    }
}

END {
    for (x in num) {
        split(x, name, "/");
        printf("%f %f %s %s\n", num[x]/tot, hits[x]/num[x], name[2], name[3])
    }
}
