//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // in seconds (������Ϊ��λ)
	float DeltaTime()const; // in seconds (������Ϊ��λ)

	void Reset(); // Call before message loop.  (�ڿ�ʼ��Ϣѭ��֮ǰ����)
	void Start(); // Call when unpaused.        (�����ʱ����ͣʱ����)
	void Stop();  // Call when paused.          (��ͣ��ʱ��ʱ����)
	void Tick();  // Call every frame.          (ÿ֡��Ҫ����)

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H