#!/usr/bin/awk -f
BEGIN {
    prod = 1;
    count = 0;
}
{
    prod *= $1;
    count++;
}
END {
    print prod ^ (1 / count);
}
