//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
float GameTimer::TotalTime()const
{
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
    // 
    // 如果正处于停止状态，则忽略本次停止时刻至当前时刻的这段时间。此外，如果之前已有过暂停的情况，
    // 那么也不应统计mStopTime – mBaseTime这段时间内的暂停时间
    // 为了做到这一点，可以从mStopTime中再减去暂停时间mPausedTime
    //
    //                            前一次暂停时间　　　　　　　　   当前的暂停时间
    //                         |<--------------->|              |<---------->|
    // ----*---------------*-----------------*------------*------------*------> 时间
    // mBase Time      mStopTime0            startTime    mStopTime     mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
    // 
    // 我们并不希望统计mCurrTime – mBaseTime内的暂停时间
    // 可以通过从mCurrTime中再减去暂停时间mPausedTime来实现这一点
    //
    // (mCurrTime - mPausedTime) - mBaseTime 
    //
    //                       |<--  暂停时间  -->|
    // ----*---------------*-----------------*------------*------> 时间
    // mBaseTime        mStopTime          startTime     mCurrTime

	else
	{
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     
    // 
    // 累加调用stop和start这对方法之间的暂停时刻间隔
    //
    //                         |<-------d------->|
    // ----*---------------*-----------------*------------> 时间
    // mBase Time        mStopTime            startTime
    // 
    // 如果从停止状态继续计时的话……
	if( mStopped )
	{
        // 累加暂停时间
		mPausedTime += (startTime - mStopTime);	

        // 在重新开启计时器时，前一帧的时间mPrevTime是无效的，这是因为它存储的是暂停时前一
        // 帧的开始时刻，因此需要将它重置为当前时刻
		mPrevTime = startTime;

        // 已不再是停止状态……
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
    // 如果已经处于停止状态，那就什么也不做
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        // 否者，保存停止的时刻，并设置布尔标志，指示计时器已经停止
		mStopTime = currTime;
		mStopped  = true;
	}
}

/*
    当渲染动画帧时，我们需要知道每帧之间的时间间隔，以此来根据时间的流逝对游戏对象进行更新。计算帧与帧之间间隔的流程如下。
    假设在开始显示第[插图]帧画面时，性能计数器返回的时刻为[插图]；而此前的一帧开始显示时，性能计数器返回的时刻为[插图]。
    那么，这两帧的时间间隔就是[插图]。对于实时渲染来说，为了保证动画的流畅性至少需要每秒刷新30帧（实际上通常会采用更高的帧率），
    所以[插图]往往是个较小的数值。
*/
void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

    // 获得本帧开始显示的时刻
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// Time difference between this frame and the previous.
    // 本帧与前一帧的时间差
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// Prepare for next frame.
    // 准备计算本帧与下一帧的时间差
	mPrevTime = mCurrTime;

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
    // 使时间差为非负值。DXSDK中的CDXUTTimer示例注释里提到：如果处理器处于节能模式，或者在
    // 计算两帧间时间差的过程中切换到另一个处理器时（即QueryPerformanceCounter函数的两次调
    // 用并非在同一处理器上），则mDeltaTime有可能会成为负值
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

/*
    4.4　计时与动画
        为了制作出精准的动画效果就需要精确地计量时间，特别是要准确地度量出动画每帧画面之间的时间间隔。
        如果帧率（frame rate，也有作帧速率、帧频等，每秒刷新的帧数）较高，那么帧与帧之间的间隔就会比较短，
        此时我们就要用到高精度的计时器。
    4.4.1　性能计时器为了精确地度量时间，我们将采用性能计时器（performance timer。
        或称性能计数器，performance counter）。如果希望调用查询性能计时器的Win32函数，我们必须引入头文件#include <windows.h>。
        性能计时器所用的时间度量单位叫作计数（count）。可调用QueryPerformanceCounter函数来获取性能计时器测量的当前时刻值（以计数为单位）：
            __int64 currTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        观察可知，此函数通过参数返回的当前时刻值是个64位的整数。再用QueryPerformanceFrequency函数来获取性能计时器的频率（单位：计数/秒）：
            __int64 countsPerSec;
            QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

        每个计数所代表的秒数（或称几分之一秒），即为上述性能计时器频率的倒数：
            mSecondsPerCount = 1.0 / (double)countsPerSec;

        因此，只需将读取的时刻计数值valueInCounts乘以转换因子mSecondsPerCount，就可以将其单位转换为秒：
            valueInSecs = valueInCounts * mSecondsPerCount;

        对我们而言，单次调用QueryPerformanceCounter函数所返回的时刻值并没有什么特别的意义。
        如果隔一小段时间，再调用一次该函数，并得到此时的时刻值，我们就会发现这两次调用的时刻间隔即为两个返回值的差。
        因此，我们总是以两个时间戳（time stamp）的相对差值，而非性能计数器单次返回的实际值来度量时间。通过下面的代码来明确这一想法：
            __int64 A = 0;
            QueryPerformanceCounter((LARGE_INTEGER*)&A);

            // 执行预定的逻辑

            __int64 B = 0;
            QueryPerformanceCounter((LARGE_INTEGER*)&B);
            (利用(B-A)即可获得代码执行期间的计数值，或以(B-A)*mSecondsPerCount获取代码运行期间所花费的秒数[41]。)

*/