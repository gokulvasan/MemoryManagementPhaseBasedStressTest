#!/bin/bash

# Generate files and corresponding structure.
# corresponding structure needs to be redirected to files.h

scale=1048576
persizecount=50000
SizeGranularity='K'
disksize=0
diskpath="./test.img"
mountpath="./G"

multiplier=2
MEGABYTE=1048576
KILOBYTE=1024

if [ $SizeGranularity == 'M' ]; then
	scale=$MEGABYTE
elif [ $SizeGranularity == 'K' ]; then
	scale=$KILOBYTE
else
	echo "Granularity Error"
	exit -1
fi

declare -a list_type=()

for Item in 'a'
do 
       list_type+=(Item$Item)
done

echo "/* Auto Generating test files\n"
# ASSUMPTION: This simply calculates count of SizeGranularity 
j=1
disksize=0
for Item in ${list_type[*]}
do 
   size=$((4 * $j))
   disksize=$(( $disksize + $(( $size * $persizecount )) ))
   #echo "$size * $persizecount"
   #echo $disksize
   j=$(( $j * $multiplier ))
done
# Some Buffer
disksize=$(( $disksize * $multiplier ))
# disksize= $(( $disksize / $scale )) 2>/dev/null
echo $disksize 
if [ ! -e $mountpath ]
then
     mkdir $mountpath
     chmod 777 $mountpath
fi

dd if="/dev/zero" bs=1$SizeGranularity count=$disksize of=$diskpath
chmod 777 $diskpath
mkfs.ext3 $diskpath 2>/dev/null 3>/dev/null

mount -v $diskpath $mountpath >/dev/null 
if [ $? -ne 0 ] 
then
	echo "Mount Failure"
	exit -1
fi

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
   size=$(( 4 * $j ))
   size=$size$SizeGranularity
   for i in $(seq 1 1 $persizecount) 
   do
       dd if=/dev/zero bs=$size count=1 of=$mountpath"/"$size"_"$i.txt 2>/dev/null
	if [ $? -ne 0 ]; then
		echo "File Creation Failure @ $mountpath"/"$size"_"$i.txt"
		exit -1
	fi
       # use chown here but not needed now
       chmod 775 $mountpath"/"$size"_"$i.txt	
       if [ $? -ne 0 ]; then
            echo "chmod failure @ $mountpath"/"$size"_"$i.txt"
            exit -1
       fi

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
   echo -e "\t {$k, file_$type, 0},"
   j=$(($j * $multiplier))
done
echo "};"

umount $mountpath
rm -rf $mountpath
