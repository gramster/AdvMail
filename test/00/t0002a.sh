#!/bin/sh
#
# Test folder retrievals

echo "Test0002"
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
    echo "Test0002 PASSED"
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
Fatal Error: ual_recvreply failed
Error Group: 3  (Network error)
Error Reason: 73 (Connection reset by peer)
Error Reason1: 7
foo
if [ $? -ne 0 ]; then fail; fi

$here/mtest ct gram gram > test.out  2>&1
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
