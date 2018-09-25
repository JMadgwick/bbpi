## bbpi - Calculate Pi Hex Digits in Parallel
**bbpi** is a simple program to calculate Pi Hex Digits in parallel, it uses the Bailey Borwein Plouffe formula.

It is a multi-threaded program and currently it runs on CPUs and some AMD GPUs (via the HC API).

#### How does it work?  
Take a look at my [blog](http://madgwick.xyz/bbp-pi-parallel-calculation.php) where there's further information and a link to the paper containing the algorithms I used.

#### What does it do?
By default it computes the 10 Millionth(10^7) hexadecimal digit of Pi (plus a few after that). And it doesn't take very long to do it, sub 10 seconds and fewer than 3 seconds with a good fast CPU. It can also run on some AMD GPUs, an R9 290 (from 2013) takes only ~1.5 secs to compute to the same accuracy.

#### How can I run it?
The easiest way is by downloading a release and running that. The other way is by compiling it yourself. This allows you to change the targeted hexadecimal place. You can also compile with `march=native` to reduce runtimes (by a little bit).

##### Build Instructions (CPU)
A compiler supporting C++11 is required.

On GNU/Linux:  
`g++ -pthread -std=c++11 -Ofast bbp-pi-parallel-cpu.cpp -o cpubbp -lm -lpthread`    
For a little more info see [BUILDING-LINUX](https://github.com/JMadgwick/bbpi/blob/master/BUILDING-LINUX).

On Windows:   
`cl /EHsc /O2 bbp-pi-parallel-cpu.cpp`   
For Windows I would recommend using clang instead because the binaries it makes are much faster than cl. For more info and some runtime speed comparisons see [BUILDING-WIN10](https://github.com/JMadgwick/bbpi/blob/master/BUILDING-WIN10.

If for some reason `std::thread::hardware_concurrency()` is not supported properly and the number of threads is not detected then you should be able to replace all references to it with an integer. You could also do this if you wanted to run fewer/more threads etc.

##### Build Instructions (GPU)
AMDs [ROCm](https://github.com/RadeonOpenCompute/ROCm) must be installed and working and the GPU used must be supported by HCC (gfx701,gfx803, gfx900,gfx906 at the time of writing). ROCm only works on GNU/Linux. There are no plans for it to support windows.   
Build command: ``hcc `hcc-config --cxxflags --ldflags` -Ofast bbp-pi-parallel-gpu.cpp -o gpubbp``

##### The Files
+ bbp-pi-parallel-cpu.cpp  
This is the code for the multi-threaded CPU implementation.
+ bbp-pi-parallel-gpu.cpp   
This is the code for the HC GPU implementation.


###### The program in action
Here is a video demonstrating an earlier build of the CPU and GPU implementation.
[![Alt text](https://img.youtube.com/vi/kC9i8Almlbg/0.jpg)](https://www.youtube.com/watch?v=kC9i8Almlbg)
