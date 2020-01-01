for i in {1..100}
do
 ./bench10.sh ./wc2a -lwc $1
done

for i in {1..100}
do
 ./bench10.sh ./wc2a -lwcP $1
done

for i in {1..100}
do
 ./bench10.sh ./wc2u -lwm $1
done

for i in {1..100}
do
 ./bench10.sh ./wc2u -lwmP $1
done
