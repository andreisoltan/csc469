# $1 - $5 files
# $6 - filter
# $7 - ASLR filter setting


pat=$6
aslr=$7

v1=`paste <(cat $1 | grep $pat | awk -v a=$aslr 'NR {if($2 == 2 && $3 == a) print $1, $3, $4, $5}') <(cat $1 | grep $pat | awk -v a=$aslr 'NR {if($2 == 3 && $3 == a) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=($5==0?0:$4/$5); sum+=diff; count++;} END {print (count==0?0:sum/count)}'`
v2=`paste <(cat $2 | grep $pat | awk -v a=$aslr 'NR {if($2 == 2 && $3 == a) print $1, $3, $4, $5}') <(cat $2 | grep $pat | awk -v a=$aslr 'NR {if($2 == 3 && $3 == a) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=($5==0?0:$4/$5); sum+=diff; count++;} END {print (count==0?0:sum/count)}'`
v3=`paste <(cat $3 | grep $pat | awk -v a=$aslr 'NR {if($2 == 2 && $3 == a) print $1, $3, $4, $5}') <(cat $3 | grep $pat | awk -v a=$aslr 'NR {if($2 == 3 && $3 == a) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=($5==0?0:$4/$5); sum+=diff; count++;} END {print (count==0?0:sum/count)}'`
v4=`paste <(cat $4 | grep $pat | awk -v a=$aslr 'NR {if($2 == 2 && $3 == a) print $1, $3, $4, $5}') <(cat $4 | grep $pat | awk -v a=$aslr 'NR {if($2 == 3 && $3 == a) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=($5==0?0:$4/$5); sum+=diff; count++;} END {print (count==0?0:sum/count)}'`
v5=`paste <(cat $5 | grep $pat | awk -v a=$aslr 'NR {if($2 == 2 && $3 == a) print $1, $3, $4, $5}') <(cat $5 | grep $pat | awk -v a=$aslr 'NR {if($2 == 3 && $3 == a) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=($5==0?0:$4/$5); sum+=diff; count++;} END {print (count==0?0:sum/count)}'`

(echo $v1; echo $v2; echo $v3; echo $v4; echo $v5) | awk 'BEGIN{sum=0; count=0;} NR{sum+=$1; count++;} END{printf "%5f", sum/count}'


