#!/bin/sh
#

echo "Test0008"
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
    echo "Test0008 PASSED"
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
     15      36     312
foo
if [ $? -ne 0 ]; then fail; fi

rm /tmp/crap
$here/mtest -P "wc>/tmp/crap"  ct "Graham Whee" gram in:1 > /dev/null
if [ $? -ne 0 ]; then fail; fi
diff /tmp/crap  test.ok
if [ $? -ne 0 ]; then fail; fi

pass
