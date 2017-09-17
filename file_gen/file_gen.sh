#!/bin/bash

# Generate files and corresponding structure.
# corresponding structure needs to be redirected to files.h

multiplier=2
scale=1048576
persizecount=50

disksize=0
diskpath="./test.img"
mountpath="./G"

declare -a list_type=()
j=1

for Item in 'a' 'b' 'c' 'd' 'e'
do 
       list_type+=(Item$Item)
done

echo "/* Auto Generating test files\n"
for Item in ${list_type[*]}
do 
   size=$((4 * $j))
   disksize=$(($disksize + $(($size * $persizecount)) ))
   j=$(($j * $multiplier))
done

disksize= $(( $disksize / 1024 )) 2>/dev/null
#echo $disksize 
if [ ! -e $mountpath ]
then
     mkdir $mountpath
     chmod 777 $mountpath
fi

dd if="/dev/zero" bs=1M count=$disksize of=$diskpath 2>/dev/null 3>/dev/null
chmod 777 $diskpath
mkfs.ext3 $diskpath 2>/dev/null 3>/dev/null

mount -v $diskpath $mountpath >/dev/null 

echo "*/"

j=1
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
   for i in $(seq 1 1 $persizecount) 
   do
       dd if=/dev/zero ibs=$size count=1 of=$mountpath"/"$size"_"$i.txt 2>/dev/null
       # use chown here but not needed now
       chmod 777 $mountpath"/"$size"_"$i.txt
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

umount $mountpath
rm -rf $mountpath
