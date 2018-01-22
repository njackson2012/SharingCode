sum=0
for f in $(find "$1" -name "*.c" -o -name "*.s" -o -name "*.h")
do
	sum="$(($(cat $f | wc -l) + $sum))"
done
echo $sum
