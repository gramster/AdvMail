#!/bin/sh
#
# Test server connections

echo "Test0001"
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
    echo "Test0001 PASSED"
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
Useage: mtest [opts] <server> <user> <password> [<spec>...]
	where <spec> has the form <fname>[:<item>...]
	and <fname> is one of 'in', 'out', 'pending', 'list', 'file' or 'bbs'
Options are:
	-p <password>	Change password
	-P <cmd>	Print item using given command
	-s <filename>	Save specified item to file
	-S <filename>	Save specified item to file as text

all, -P, -s and -S are mutually exclusive
foo
if [ $? -ne 0 ]; then fail; fi

$here/mtest  > test.out  2>&1
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
