# $1 - $5 files
# $6 - filter


pat=$6

v1=`paste <(cat $1 | grep $pat | awk 'NR {if($2 == 2) print $1, $3, $4, $5}') <(cat $1 | grep $pat | awk 'NR {if($2 == 3) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++;} END {print sum/count}'`
v2=`paste <(cat $2 | grep $pat | awk 'NR {if($2 == 2) print $1, $3, $4, $5}') <(cat $2 | grep $pat | awk 'NR {if($2 == 3) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++;} END {print sum/count}'`
v3=`paste <(cat $3 | grep $pat | awk 'NR {if($2 == 2) print $1, $3, $4, $5}') <(cat $3 | grep $pat | awk 'NR {if($2 == 3) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++;} END {print sum/count}'`
v4=`paste <(cat $4 | grep $pat | awk 'NR {if($2 == 2) print $1, $3, $4, $5}') <(cat $4 | grep $pat | awk 'NR {if($2 == 3) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++;} END {print sum/count}'`
v5=`paste <(cat $5 | grep $pat | awk 'NR {if($2 == 2) print $1, $3, $4, $5}') <(cat $5 | grep $pat | awk 'NR {if($2 == 3) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++;} END {print sum/count}'`

(echo $v1; echo $v2; echo $v3; echo $v4; echo $v5) | awk 'BEGIN{sum=0; count=0;} NR{sum+=$1; count++;} END{print sum/count}'

