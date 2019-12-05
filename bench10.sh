export LC_CTYPE=C.UTF-8
export TIMEFORMAT=$1,$2,$3,%U
{ bash -c "time $1 $2 $3 $3 $3 $3 $3 $3 $3 $3 $3 $3" >/dev/null ; } 2>&1
