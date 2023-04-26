#!/bin/sh
#

echo "Test0007"
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
    echo "Test0007 PASSED"
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

2 Item(s)
 1    object               Graham Whee /CT   Message     
O2    test                 Ian Smith /CT       Message     
Disconnected
File exists and overwrite not allowed
Connected
----------------------------
Folder In Tray

2 Item(s)
 1    object               Graham Whee /CT   Message     
O2    test                 Ian Smith /CT       Message     
Disconnected
MESSAGE                                                 Dated: 06/01/95 at 23:12
Subject: object                                                      Contents: 2
Sender:  Graham Whee / CT

Part 1

  TO: Graham Whee / CT

Part 2

rhubarb rhubarb rhubarb
foo
if [ $? -ne 0 ]; then fail; fi
rm /tmp/crap
$here/mtest -S /tmp/crap ct "Graham Whee" gram in:1 > test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
$here/mtest -S /tmp/crap ct "Graham Whee" gram in:1 >> test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
cat /tmp/crap >> test.out
if [ $? -ne 0 ]; then fail; fi
rm /tmp/crap
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
