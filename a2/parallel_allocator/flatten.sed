/--/ {
    N
    s/--\n//
}

/name/ {
    N
    N
    s/name = \([-a-z]*\)\n.*scalability score \([0-9.]*\)\n.*fragmentation score = \([0-9.]*\)/\1 \2 \3/
}
