#!/bin/sh
#

echo "Test0006"
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
    echo "Test0006 PASSED"
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
Disconnected
Connected
Disconnected
foo
if [ $? -ne 0 ]; then fail; fi

$here/mtest -p abc ct "Ian Smith" pt92s > test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
$here/mtest -p pt92s ct "Ian Smith" abc >> test.out 2>&1
if [ $? -ne 0 ]; then fail; fi
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
