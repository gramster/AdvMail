#!/bin/sh
#

echo "Test0005"
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
    echo "Test0005 PASSED"
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
No lines read: EOF has already been reached
Connected
----------------------------
Folder File Area

3 Item(s)
 1    test1                Ian Smith /CT       Folder      
 2    Message Log          Ian Smith /CT       Folder      
 3    WASTE BASKET         Ian Smith /CT       Folder      
----------------------------
Folder Message Log

6 Item(s)
 1    file                 Ian Smith /CT       Message     
 2    test                 Ian Smith /CT       Message     
 3    test again           Ian Smith /CT       Message     
 4    test again           Ian Smith /CT       Message     
 5    test                 Ian Smith /CT       Message     
 6    test                 Ian Smith /CT       Message     
----------------------------
Message test

1:1> MESSAGE                                                 Dated: 04/20/95 at 13:13
1:2> Subject: test                                                        Contents: 3
1:3> Creator: Ian Smith / CT
1:4> 
1:5> Part 1
1:6> 
1:7>   TO: Ian Smith / CT
1:8> 
1:9> Part 2
1:10> 
2:1> 
2:2> Part 3
2:3> 
2:4> This item is of type BINARY FILE and cannot be displayed as TEXT
2:5> 
Disconnected
foo
if [ $? -ne 0 ]; then fail; fi

$here/mtest ct "Ian Smith" pt92s file:2:2 > test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
