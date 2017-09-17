#!/bin/bash

multiplier=2
scale=1048576



declare -a list_type=()
j=1

for Item in 'a' 'b' 'c' 'd' 'e'
do 
       list_type+=(Item$Item)
done

echo "typedef enum file_len {"
for item in ${list_type[*]}
do
	echo -e "\t $item ,"
done
echo -e "\t max"
echo "}file_len_t;"

for type in ${list_type[*]}
do
   echo "static file_path file_$type[] = { "
   size=$((4 * $j))
   size=$size'M'
   for i in $(seq 1 1 50) 
   do
       dd if=/dev/zero ibs=$size count=1 of=$size"_"$i.txt 2>/dev/null
       echo -e "\t {0, PATH($size"_"$i.txt)},"
   done
   echo -e "\t {0, NULL},"
   echo -e "};\n"
   j=$(($j * $multiplier))
done

j=1
echo "file_lst_size_t file_lst[max] = { "
for type in ${list_type[*]}
do 
   k=$((4 * $j * $scale ))
   echo -e "\t {$k, file_$type},"
   j=$(($j * $multiplier))
done
echo "};"
