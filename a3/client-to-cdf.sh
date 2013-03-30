#/bin/bash

CDF_INVOK=`wget -O - http://www.cdf.toronto.edu/~csc469h/winter/chatserver.txt | awk '{print "./chatclient -h", $1, "-t", $2, "-u", $3 }'`

$CDF_INVOK -n $1
