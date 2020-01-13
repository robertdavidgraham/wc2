
bench() {
    export TIMEFORMAT=%U,$1,$2,$3
    for i in $(seq 10)
    do
        { bash -c "time $1 $2 $3 $3 $3 $3 $3 $3 $3 $3 $3 $3" >/dev/null ; } 2>&1
    done | sort -n | head -n 1
}
export LC_CTYPE=en_US.UTF-8
locale | grep LC_CTYPE 1>&2
bench ./wc2 -lwm space.txt
bench ./wc2 -lwm word.txt
bench ./wc2 -lwm ascii.txt
bench ./wc2 -lwm utf8.txt
bench ./wc2 -lwm pocorgtfo18.pdf


bench ./wc2.js -lwm space.txt
bench ./wc2.js -lwm word.txt
bench ./wc2.js -lwm ascii.txt
bench ./wc2.js -lwm utf8.txt
bench ./wc2.js -lwm pocorgtfo18.pdf

bench wc -lwm space.txt
bench wc -lwm word.txt
bench wc -lwm ascii.txt
bench wc -lwm utf8.txt
bench wc -lwm pocorgtfo18.pdf
