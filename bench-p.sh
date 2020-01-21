
bench() {
    export TIMEFORMAT=%U,$1,$2,$3
    for i in $(seq 100)
    do
        { bash -c "time $1 $2 $3 $3 $3 $3 $3 $3 $3 $3 $3 $3" >/dev/null ; } 2>&1
    done | sort -n | head -n 1
}
export LC_CTYPE=en_US.UTF-8
./wctool --locale
bench ./wc2 -lwm pocorgtfo18.pdf
bench ./wc2 -lwmP pocorgtfo18.pdf
