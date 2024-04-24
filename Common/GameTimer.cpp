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
    // ���������ֹͣ״̬������Ա���ֹͣʱ������ǰʱ�̵����ʱ�䡣���⣬���֮ǰ���й���ͣ�������
    // ��ôҲ��Ӧͳ��mStopTime �C mBaseTime���ʱ���ڵ���ͣʱ��
    // Ϊ��������һ�㣬���Դ�mStopTime���ټ�ȥ��ͣʱ��mPausedTime
    //
    //                            ǰһ����ͣʱ�䡡��������������   ��ǰ����ͣʱ��
    //                         |<--------------->|              |<---------->|
    // ----*---------------*-----------------*------------*------------*------> ʱ��
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
    // ���ǲ���ϣ��ͳ��mCurrTime �C mBaseTime�ڵ���ͣʱ��
    // ����ͨ����mCurrTime���ټ�ȥ��ͣʱ��mPausedTime��ʵ����һ��
    //
    // (mCurrTime - mPausedTime) - mBaseTime 
    //
    //                       |<--  ��ͣʱ��  -->|
    // ----*---------------*-----------------*------------*------> ʱ��
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
    // �ۼӵ���stop��start��Է���֮�����ͣʱ�̼��
    //
    //                         |<-------d------->|
    // ----*---------------*-----------------*------------> ʱ��
    // mBase Time        mStopTime            startTime
    // 
    // �����ֹͣ״̬������ʱ�Ļ�����
	if( mStopped )
	{
        // �ۼ���ͣʱ��
		mPausedTime += (startTime - mStopTime);	

        // �����¿�����ʱ��ʱ��ǰһ֡��ʱ��mPrevTime����Ч�ģ�������Ϊ���洢������ͣʱǰһ
        // ֡�Ŀ�ʼʱ�̣������Ҫ��������Ϊ��ǰʱ��
		mPrevTime = startTime;

        // �Ѳ�����ֹͣ״̬����
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
    // ����Ѿ�����ֹͣ״̬���Ǿ�ʲôҲ����
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        // ���ߣ�����ֹͣ��ʱ�̣������ò�����־��ָʾ��ʱ���Ѿ�ֹͣ
		mStopTime = currTime;
		mStopped  = true;
	}
}

/*
    ����Ⱦ����֡ʱ��������Ҫ֪��ÿ֮֡���ʱ�������Դ�������ʱ������Ŷ���Ϸ������и��¡�����֡��֮֡�������������¡�
    �����ڿ�ʼ��ʾ��[��ͼ]֡����ʱ�����ܼ��������ص�ʱ��Ϊ[��ͼ]������ǰ��һ֡��ʼ��ʾʱ�����ܼ��������ص�ʱ��Ϊ[��ͼ]��
    ��ô������֡��ʱ��������[��ͼ]������ʵʱ��Ⱦ��˵��Ϊ�˱�֤������������������Ҫÿ��ˢ��30֡��ʵ����ͨ������ø��ߵ�֡�ʣ���
    ����[��ͼ]�����Ǹ���С����ֵ��
*/
void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

    // ��ñ�֡��ʼ��ʾ��ʱ��
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// Time difference between this frame and the previous.
    // ��֡��ǰһ֡��ʱ���
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// Prepare for next frame.
    // ׼�����㱾֡����һ֡��ʱ���
	mPrevTime = mCurrTime;

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
    // ʹʱ���Ϊ�Ǹ�ֵ��DXSDK�е�CDXUTTimerʾ��ע�����ᵽ��������������ڽ���ģʽ��������
    // ������֡��ʱ���Ĺ������л�����һ��������ʱ����QueryPerformanceCounter���������ε�
    // �ò�����ͬһ�������ϣ�����mDeltaTime�п��ܻ��Ϊ��ֵ
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

/*
    4.4����ʱ�붯��
        Ϊ����������׼�Ķ���Ч������Ҫ��ȷ�ؼ���ʱ�䣬�ر���Ҫ׼ȷ�ض���������ÿ֡����֮���ʱ������
        ���֡�ʣ�frame rate��Ҳ����֡���ʡ�֡Ƶ�ȣ�ÿ��ˢ�µ�֡�����ϸߣ���ô֡��֮֡��ļ���ͻ�Ƚ϶̣�
        ��ʱ���Ǿ�Ҫ�õ��߾��ȵļ�ʱ����
    4.4.1�����ܼ�ʱ��Ϊ�˾�ȷ�ض���ʱ�䣬���ǽ��������ܼ�ʱ����performance timer��
        ������ܼ�������performance counter�������ϣ�����ò�ѯ���ܼ�ʱ����Win32���������Ǳ�������ͷ�ļ�#include <windows.h>��
        ���ܼ�ʱ�����õ�ʱ�������λ����������count�����ɵ���QueryPerformanceCounter��������ȡ���ܼ�ʱ�������ĵ�ǰʱ��ֵ���Լ���Ϊ��λ����
            __int64 currTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        �۲��֪���˺���ͨ���������صĵ�ǰʱ��ֵ�Ǹ�64λ������������QueryPerformanceFrequency��������ȡ���ܼ�ʱ����Ƶ�ʣ���λ������/�룩��
            __int64 countsPerSec;
            QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

        ÿ���������������������Ƽ���֮һ�룩����Ϊ�������ܼ�ʱ��Ƶ�ʵĵ�����
            mSecondsPerCount = 1.0 / (double)countsPerSec;

        ��ˣ�ֻ�轫��ȡ��ʱ�̼���ֵvalueInCounts����ת������mSecondsPerCount���Ϳ��Խ��䵥λת��Ϊ�룺
            valueInSecs = valueInCounts * mSecondsPerCount;

        �����Ƕ��ԣ����ε���QueryPerformanceCounter���������ص�ʱ��ֵ��û��ʲô�ر�����塣
        �����һС��ʱ�䣬�ٵ���һ�θú��������õ���ʱ��ʱ��ֵ�����Ǿͻᷢ�������ε��õ�ʱ�̼����Ϊ��������ֵ�Ĳ
        ��ˣ���������������ʱ�����time stamp������Բ�ֵ���������ܼ��������η��ص�ʵ��ֵ������ʱ�䡣ͨ������Ĵ�������ȷ��һ�뷨��
            __int64 A = 0;
            QueryPerformanceCounter((LARGE_INTEGER*)&A);

            // ִ��Ԥ�����߼�

            __int64 B = 0;
            QueryPerformanceCounter((LARGE_INTEGER*)&B);
            (����(B-A)���ɻ�ô���ִ���ڼ�ļ���ֵ������(B-A)*mSecondsPerCount��ȡ���������ڼ������ѵ�����[41]��)

*/