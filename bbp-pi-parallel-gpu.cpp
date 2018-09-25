// Parallel Implementation of the Bailey–Borwein–Plouffe Formula for Pi
// Copyright (C) 2018 J. Madgwick

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <iostream>
#include <cmath>
#include <string>
//for printout accuracy
#include <limits>
#include <iomanip>
//CPU Multithreading
#include <thread>
#include <future>
//Header files for the hc API
#include <hcc/hc.hpp>
#include <hcc/hc_math.hpp>

#pragma GCC diagnostic warning "-Wall"//Gives us 'all' warnings, includes avoiding uninitialized varibles, needed for HCC compiler which treats them strangly.

//Left-Right Binary algorithm for exponentiation with Modulo 16^n mod k
double expoMod(double n, double k)
  {
    static int init = 0;//Store whether table initialised
    static int pwrtbl[32];//table to store powers of 2
    static int highestpwrtblpwr = 0;//after init, points to largest value in the table, which is <= n
    while (!init) //Find largest power needed, as n counts down, do this only once
    {
      pwrtbl[0] = 1; //In theory first can be 0 but in practice this function is only called if n > 0, in which case greatest power is 1 or more
      while (pwrtbl[highestpwrtblpwr] < n) //used instead of for loop in order to check before iterating
      {
        pwrtbl[highestpwrtblpwr+1] = pwrtbl[highestpwrtblpwr] * 2; //only generates up to and including n, nothing larger
        highestpwrtblpwr++;
      }
      init = 1; //set table as initialised
    }

    int bitsneeded = 0;//The number of binary positions and the location in the table of the highest needed power
    for (; pwrtbl[bitsneeded]<n;bitsneeded++);//Move through table to find position of the largest power of two < n
    if (pwrtbl[bitsneeded]!=n){bitsneeded--;}//because the increment is applied before the condition check, we need to decrement by one, unless the item was equal to n

    //First set t to be the largest power of two such that t ≤ n, and set r = 1.
    int t = pwrtbl[bitsneeded];
    double r = 1;
    
    //Loop by the number of Binary positions
    for (int i = 0; i <= bitsneeded; i++) 
    {
      if (n>=t) // if n ≥ t then
      {
        r = r * 16.0;// r ← br
        r = r - static_cast<int>(r/k)*k;// r ← r mod k
        n = n - t;// n ← n − t
      }
      t = t/2;// t ← t/2
      if (t>=1) // if t ≥ 1 then
      {
        r = r * r;// r ← r^2
        r = r - static_cast<int>(r/k)*k;// r ← r mod k
      }
    }
    return r;
  }

//Left Portion for one thread - just does one term
double leftPortionThreaded(int terms, int k, int j, int d) [[hc]]
{
  int kinit = k;
  double s = 0;
  for (;k < (kinit+terms);k++) //Always go from 0-k+terms, DONT use K!
  {
    double numerator,denominator;
    denominator = 8 * k + j;
    numerator = expoMod(d - k, denominator); // Binary algorithm for exponentiation with Modulo must be used becuase otherwise 16^(d-k) can be very large and overflows
    s = s + numerator/denominator;
    s = s - static_cast<int>(s); // Can also use 'floor' but no effect on performance
  }
  return s;
}

//Bailey–Borwein–Plouffe Formula 16^d x Sj
double bbpf16jsd(int j, int d)
  {
    double s = .0;
    double numerator,denominator;
    double term;
    int noOfThreads = 700;//static_cast<int>(std::thread::hardware_concurrency());
    int perThreadRuns = 1000;
    //std::thread *threadArray = new std::thread[noOfThreads];// Array with the number of threads avalible
    double threadResults[noOfThreads];// For storing results from threads
    std::generate_n(threadResults,noOfThreads,[]() {return (int)0;}); //Fill 
    hc::array_view<double,1> gpu_threadResults(noOfThreads,threadResults);
    //Left Portion
    int k = 0;
    while (k < d)
    {
      if (k + (perThreadRuns*noOfThreads) < d)//Only make threads for k up to less than d
      {
        hc::parallel_for_each(gpu_threadResults.get_extent(), [=] (hc::index<1> idx) [[hc]] //create and execute (noOfThreads) threads
        {
          int k2 = k+(idx[0]*perThreadRuns); //Thread 0 starts at k+0, thread 1 at k+(1*perThreadRuns) &c.
          gpu_threadResults[idx] = leftPortionThreaded(perThreadRuns,k2,j,d);
        }
        );
        k = k + perThreadRuns*noOfThreads; //We need to run (perThreadRuns) results on each thread because the thread overhead is much to great to run just 1
        for (int i2 = noOfThreads-1; i2 > -1;i2--) //fetch results from all threads - count backwards because last thread executed will be slowest (higher numbers)
        {
          ///threadArray[i2].join();
          s = s + gpu_threadResults[i2];
          s = s - static_cast<int>(s);
        }
      } else //if we are almost done and k + noOfThreads > d then do the last few terms single threaded
      {
        denominator = 8 * k + j;
        numerator = expoMod(d - k, denominator); // Binary algorithm for exponentiation with Modulo must be used becuase otherwise 16^(d-k) can be very large and overflows
        term = numerator/denominator;
        s = s + term;
        s = s - static_cast<int>(s);
        k++;
      }
    }
    //Right Portion
    for (int k = d; k <= d+100; k++) 
    {
      numerator = pow(16, d - k);
      denominator = 8 * k + j;
      term = numerator/denominator;
      if (term<1e-17) {break;}
      s = s + term;
      s = s - static_cast<int>(s);
    }
    return s;
  }

//Bailey–Borwein–Plouffe Formula Calculation
void bbpfCalc(double *pidec,int *place)
  {
    int tempn = *place;
    double result;
    result = (4.*bbpf16jsd(1,tempn))-(2.*bbpf16jsd(4,tempn))-bbpf16jsd(5,tempn)-bbpf16jsd(6,tempn);
    result = result - static_cast<int>(result) + 1.;
    *pidec = result;
  }

void toHex(char *out, double *in) //Note the pointer '*out' points to hexOutput[] (char[]) in main(). Not possible to return char[] in C/C++
{
  char hexNumbers[] = "0123456789abcdef";
  
  for (int i = 0; i <= 12;i++)
  {
    *in = 16.0 * (*in - static_cast<int>(*in));
    out[i] = hexNumbers[static_cast<int>(*in)];
  }
}
int main() {
  std::cout << "Bailey–Borwein–Plouffe Formula for Pi" << std::endl;
  std::cout << "Built: " << __DATE__ << " " << __TIME__ << " with HCC Version: " << __hcc_major__ << "." << __hcc_minor__ << "." << __hcc_patchlevel__ << std::endl << std::endl;
  hc::accelerator GPUdevice = hc::accelerator(); //Get default accelerator, this is always the best device - static std::vector<accelerator> hc::accelerator::get_all() to get a list of all
  std::wcout << "-------- Detected HC Device Details --------" << std::endl  
  << "          Path: " << GPUdevice.get_device_path() << std::endl
  << "       Version: " << GPUdevice.get_version() << std::endl
  << "   Description: " << GPUdevice.get_description() << std::endl
  << "     Total RAM: " << GPUdevice.get_dedicated_memory()/pow(1024,2) << " (MB)" << std::endl //RAM is shown is MB output from API is bytes
  << " Compute Units: " << GPUdevice.get_cu_count() << std::endl << std::endl;
  double piDec;
  int placeArr = 10000000; //Accurate to 10000000
  bbpfCalc(&piDec,&placeArr);  
  std::cout << "Position: " << placeArr << std::endl;
  std::cout << "Pi Estimation Decimal: " << std::setprecision (std::numeric_limits<long double>::digits10 + 2) << (piDec) << std::endl;
  char hexOutput[] = "0000000000000"; //Needs to be initialised else random data is left over at the end once its been filled
  toHex(hexOutput, &piDec);
  std::cout << "Pi Estimation Hex: " << hexOutput << std::endl;
}
