#!/bin/sh
#

echo "Test0004"
tmp=/tmp/$$
here=`pwd`
if [ $? -ne 0 ]; then exit 1; fi

fail()
{
    echo FAILED 1>&2
    cd $here
    rm -rf $tmp
    exit 1
}

pass()
{
    echo "Test0004 PASSED"
    cd $here
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15

mkdir $tmp
if [ $? -ne 0 ]; then exit 1; fi
cd $tmp
if [ $? -ne 0 ]; then exit 1; fi

# What we expect:

cat > test.ok << 'foo'
Connected
----------------------------
Folder In Tray

6 Item(s)
 1    test yet again       Ian Cooper /CT       Message     
 2    testing from mobile  Ian Cooper /CT       Message     
 3    test                 Ian Cooper /CT       Message     
O4    test                 Ian Cooper /CT       Message     
O5    test                 Ian Cooper /CT       Message     
O6    ioieow               Ian Cooper /CT       Message     
----------------------------
Folder Out Tray

2 Item(s)
 1    test                 Ian Cooper /CT       Message     
 2    gyghjghj             Ian Cooper /CT       Message     
----------------------------
Folder Pending Tray

1 Item(s)
 1    testing              Ian Cooper /CT       Message     
----------------------------
Folder File Area

3 Item(s)
 1    test1                Ian Cooper /CT       Folder      
 2    Message Log          Ian Cooper /CT       Folder      
 3    WASTE BASKET         Ian Cooper /CT       Folder      
----------------------------
Folder Distribution Lists

1 Item(s)
 1    est list             Ian Cooper /CT       Distribution
----------------------------
Folder Bulletin Boards

0 Item(s)
Disconnected
foo
if [ $? -ne 0 ]; then fail; fi

$here/mtest ct "Ian Smith" pt92s all > test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
