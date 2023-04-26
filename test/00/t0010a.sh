
#!/bin/sh
#
# Test folder retrievals

echo "Test0010"
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
    echo "Test0010 PASSED"
    cd $here
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15

mkdir $tmp
if [ $? -ne 0 ]; then exit 1; fi
cd $tmp
if [ $? -ne 0 ]; then exit 1; fi

# What we send

cat > agent.rec << 'foo'
enter
F8
F8
F1
foo

# What we expect:

cat > test.ok << 'foo'
Clear
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 37 Login
Reverse on
 1  0 Copyright (c) IBM, Open Mind Solutions                                          
Reverse off
Move  0  0
Refresh
Reverse on
22  0 f1      
23  0 ESC-1   
Reverse off
Reverse on
22  9 f2      
23  9 ESC-2   
Reverse off
Reverse on
22 18 f3      
23 18 ESC-3   
Reverse off
Reverse on
22 27 f4      
23 27 ESC-4   
Reverse off
Reverse on
22 44 f5      
23 44 ESC-5   
Reverse off
Reverse on
22 53 f6      
23 53 ESC-6   
Reverse off
Reverse on
22 62 f7      
23 62 ESC-7   
Reverse off
Reverse on
22 71 f8      
23 71 ESC-8   
Reverse off
Move  0  0
15  1 Server:
16  1 User:
17  1 Enter your password:
15  9                               
16  7                                       
17 23                       
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 37 Login
Reverse on
 1  0 Checking password, please wait.                                                 
Reverse off
Move 17 45
Refresh
Clear
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 38 Main
Reverse on
 1  0 Select an area or choose a function.                                            
Reverse off
Move  0  0
Refresh
Reverse on
22  0  Select 
23  0   Area  
Reverse off
Reverse on
22  9         
23  9         
Reverse off
Reverse on
22 18         
23 18         
Reverse off
Reverse on
22 27         
23 27         
Reverse off
Reverse on
22 44         
23 44         
Reverse off
Reverse on
22 53  Config 
23 53         
Reverse off
Reverse on
22 62   Help  
23 62         
Reverse off
Reverse on
22 71   Exit  
23 71 ADV'MAIL
Reverse off
Move  0  0
 4  0 User:
 4  6 Graham Whee /CT
 5 65 Date:
 8 16 Area
 8 52 Contents
 5 71 07/06/95
10 16 Out Tray                                   2
12 16 Filing Cabinet                             1
13 16 Distribution Lists                         0
15 16 Pending Tray                               0
Reverse on
 9 16 In Tray                                    1
Reverse off
Move  9 16
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 38 Main
Reverse on
 1  0 Choose a function or press Help for more information.                           
Reverse off
Move  9 16
Refresh
Refresh
Clear
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 36 In Tray
Reverse on
 1  0 Choose a message and a function.                                                
Reverse off
Move  0  0
Refresh
Reverse on
22  0  Other  
23  0   Keys  
Reverse off
Reverse on
22  9   Read  
23  9         
Reverse off
Reverse on
22 18  Print  
23 18         
Reverse off
Reverse on
22 27  Delete 
23 27         
Reverse off
Reverse on
22 44  Reply  
23 44         
Reverse off
Reverse on
22 53 Forward 
23 53         
Reverse off
Reverse on
22 62   Help  
23 62         
Reverse off
Reverse on
22 71   Done  
23 71         
Reverse off
Move  0  0
 3 73 N A P U
 4 71 T E C R R
 5 71 O W K I G
 5  4 Subject
 5 36 Sender
 5 61 Received
 3  0 1 Message 
Reverse on
 7  0     object                          Graham Whee /CT       06/01/95  *        
Reverse off
Move  7  0
Refresh
Clear
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 38 Main
Reverse on
 1  0 Select an area or choose a function.                                            
Reverse off
Move  0  0
Refresh
Reverse on
22  0  Select 
23  0   Area  
Reverse off
Reverse on
22  9         
23  9         
Reverse off
Reverse on
22 18         
23 18         
Reverse off
Reverse on
22 27         
23 27         
Reverse off
Reverse on
22 44         
23 44         
Reverse off
Reverse on
22 53  Config 
23 53         
Reverse off
Reverse on
22 62   Help  
23 62         
Reverse off
Reverse on
22 71   Exit  
23 71 ADV'MAIL
Reverse off
Move  0  0
 4  0 User:
 4  6 Graham Whee /CT
 5 65 Date:
 8 16 Area
 8 52 Contents
 5 71 07/06/95
10 16 Out Tray                                   2
12 16 Filing Cabinet                             1
13 16 Distribution Lists                         0
15 16 Pending Tray                               0
Reverse on
 9 16 In Tray                                    1
Reverse off
Move  9 16
Refresh
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 38 Main
Reverse on
 1  0 Press Confirm Exit to leave AdvanceMail/Remote                                  
Reverse off
Move  9 16
Refresh
Reverse on
22  0 Confirm 
23  0 Exit    
Reverse off
Reverse on
22  9         
23  9         
Reverse off
Reverse on
22 18         
23 18         
Reverse off
Reverse on
22 27         
23 27         
Reverse off
Reverse on
22 44         
23 44         
Reverse off
Reverse on
22 53         
23 53         
Reverse off
Reverse on
22 62         
23 62         
Reverse off
Reverse on
22 71 Cancel  
23 71 Exit    
Reverse off
Move  9 16
Refresh
Clear
 0  0 AdvanceMail/Remote(0.1)                                                         
 0 37 Login
Reverse on
 1  0 Terminating AdvanceMail/Remote                                                  
Reverse off
Move  0  0
Refresh
Clear
foo
if [ $? -ne 0 ]; then fail; fi

$here/agent -b 
mv agent.out test.out
diff test.out test.ok
if [ $? -ne 0 ]; then fail; fi

pass
