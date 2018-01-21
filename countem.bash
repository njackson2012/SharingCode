sum=0
for f in $(find "$1")
do
	sum="$(($(cat $f | wc -l) + $sum))"
done
echo $sum
