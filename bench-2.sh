
bench() {
    export TIMEFORMAT=%U,$1,$2,$3
    for i in $(seq 10)
    do
        { bash -c "time $1 $2 $3 $3 $3 $3 $3 $3 $3 $3 $3 $3" >/dev/null ; } 2>&1
    done | sort -n | head -n 1
}
export LC_CTYPE=en_US.UTF-8
locale | grep LC_CTYPE 1>&2
bench wc -lwm space.txt
bench wc -lwm word.txt
bench wc -lwm ascii.txt
bench wc -lwm utf8.txt
bench wc -lwm pocorgtfo18.pdf

export LC_CTYPE=C
locale | grep LC_CTYPE 1>&2
bench wc -lwc space.txt
bench wc -lwc word.txt
bench wc -lwc ascii.txt
bench wc -lwc utf8.txt
bench wc -lwc pocorgtfo18.pdf

