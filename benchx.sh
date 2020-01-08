for i in {1..10}
do
 ./bench.sh ./wc2u -lwm $1
done

for i in {1..10}
do
 ./bench.sh ./wc2u -lwmP $1
done

for i in {1..10}
do
 ./bench.sh ./wc2u -lwmPP $1
done
