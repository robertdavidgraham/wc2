FILENAME=pocorgtfo18.pdf
for i in {1..100}
do
 ./bench10.sh ./wc2u -lwm $FILENAME
done

for i in {1..100}
do
 ./bench10.sh ./wc2u -lwmP $FILENAME
done

for i in {1..100}
do
 ./bench10.sh ./wc2u -lwmPP $FILENAME
done
