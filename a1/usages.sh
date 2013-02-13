#!/bin/bash 

USAGE="
    USAGE:
        $0 --help | <dir>
    
    Writes out usage messages for inclusion in report.tex in the specified
    directory, or in the current directory if none given. If more than one
    directory is given, the first will be used. If the directory does not 
    exist it will be created.
    
    --help
        Prints this message
    "

if [[ $1 == '--help' ]]; then
    echo "$USAGE" >&2
    exit 1
fi

if [[ $# == 0 ]]; then
    PRE="./"
else
    mkdir -p "$1"
    PRE="$1/"
fi

SUF=".usage"

log_usage() {
    DEST="$PRE$1$SUF"
    rm -f "$DEST"
    $1 $2 &> "$DEST"
}

log_usage ./parta 
log_usage ./do-parta.sh -h
log_usage ./find-empty-wkstn.sh --help
log_usage ./minions.sh
log_usage ./usages.sh --help
