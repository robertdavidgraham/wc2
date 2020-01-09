
bench() {
    export TIMEFORMAT=%U,$1,$2,$3
    for i in $(seq 10)
    do
        { bash -c "time $1 $2 $3 $3 $3 $3 $3 $3 $3 $3 $3 $3" >/dev/null ; } 2>&1
    done | sort -n | head -n 1
}
locale | grep LC_CTYPE 1>&2
bench ./wc2u -lwm pocorgtfo18.pdf
bench ./wc2u.js -lwm pocorgtfo18.pdf
bench wc -lwm pocorgtfo18.pdf


