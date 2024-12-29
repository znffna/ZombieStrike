///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// CGameTimer.cpp : CGameTimer Ŭ������ ���� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include <Windows.h>
#include "GameTimer.h"

CGameTimer::CGameTimer()
	: m_fSecondsPerCount(0.0), m_fDeltaTime(-1.0), m_nBaseTime(0),
	m_nPausedTime(0), m_nStopTime(0), m_nPrevTime(0), m_nCurrTime(0), m_bStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_fSecondsPerCount = 1.0 / (double)countsPerSec;
}

CGameTimer::~CGameTimer()
{
}

// Reset()�� ȣ��� ���� ����� �� �ð��� ��ȯ�Ѵ�.
// �ð��� �����Ǿ� �ִ� ������ �ð��� �������� �ʴ´�.
float CGameTimer::GameTime() const
{
	// Ÿ�̸Ӱ� ���� �����̸�, ������ �������� �帥 �ð��� ������� ���ƾ� �Ѵ�.
	// ����, ������ �̹� �Ͻ� ������ ���� �ִٸ� �ð��� m_nStopTime - m_nBaseTime����
	// �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִµ�, �� ���� �ð��� ��ü �ð��� �������� 
	// ���ƾ� �Ѵ�. �̸� �ٷ���� ����, m_nStopTime���� �Ͻ� ���� ���� �ð���
	// ����.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if (m_bStopped)
	{
		return (float)(((m_nStopTime - m_nPausedTime)
			- m_nBaseTime) * m_fSecondsPerCount);
	}

	// �ð��� m_nCurrTime - m_nBaseTime���� �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִ�. �̸�
	// ��ü �ð��� �����ϸ� �� �ǹǷ�, �� �ð��� m_CurrTime���� ���ش�.
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime

	else
	{
		return (float)(((m_nCurrTime - m_nPausedTime) - m_nBaseTime) * m_fSecondsPerCount);
	}
}

float CGameTimer::DeltaTime() const
{
	return (float)m_fDeltaTime;
}

void CGameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_nBaseTime = currTime;
	m_nPrevTime = currTime;
	m_nStopTime = 0;
	m_bStopped = false;
}

void CGameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// ����(�Ͻ� ����)�� ����(�簳) ���̿� �帥 �ð��� �����Ѵ�.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// ���� ���¿��� Ÿ�̸Ӹ� �簳�ϴ� ��� :
	if (m_bStopped)
	{
		// �Ͻ� ������ �ð��� �����Ѵ�.
		m_nPausedTime += (startTime - m_nStopTime);

		// Ÿ�̸Ӹ� �� �ӽ����ϴ� ���̹Ƿ�, ������ m_nPrevTime(���� �ð�)��
		// ��ȿ���� �ʴ�(�Ͻ� ���� ���߿� ���ŵǾ��� ���̹Ƿ�).
		// ���� ���� �ð����� �ٽ� �����Ѵ�.
		m_nPrevTime = startTime;

		// ������ ���� ���°� �ƴϹǷ� ���� ������� �����Ѵ�.
		m_nStopTime = 0;
		m_bStopped = false;
	}
}

void CGameTimer::Stop()
{
	// �̹� ���� �����̸� �ƹ� �ϵ� ���� �ʴ´�.
	if (!m_bStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		// �׷��� �ʴٸ� ���� �ð��� Ÿ�̸� ���� ���� �ð����� �����ϰ�,
		// Ÿ�̸Ӹ� �����Ǿ����� ���ϴ� �ο� �÷��׸� �����Ѵ�.
		m_nStopTime = currTime;
		m_bStopped = true;
	}
}

void CGameTimer::Tick(float fLockFPS)
{
	if (m_bStopped)
	{
		m_fDeltaTime = 0.0;
		return;
	}

	// �̹� �������� �ð��� ��´�..
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_nCurrTime = currTime;

	// �̹� �������� �ð��� ���� �������� �ð��� ���̸� ���Ѵ�.
	m_fDeltaTime = (m_nCurrTime - m_nPrevTime) * m_fSecondsPerCount;
	if (fLockFPS > 0.0f)
	{
		while (m_fDeltaTime < (1.0f / fLockFPS))
		{
			::QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
			m_nCurrTime = currTime;
			m_fDeltaTime = float((m_nCurrTime - m_nPrevTime) * m_fSecondsPerCount);
		}
	}
	recordTimeDifference(m_fDeltaTime);

	// ���� �������� �غ��Ѵ�.
	m_nPrevTime = m_nCurrTime;

	// ������ ���� �ʰ� �Ѵ�.  SDK ����ȭ�� CDXUTTimer �׸� ������,
	// ���μ����� ���� ���� ���ų� ������ �ٸ� ���μ�����
	// ��Ű�� ���  mDeltaTime�� ������ �� �� �ִ�.
	if (m_fDeltaTime < 0.0)
	{
		m_fDeltaTime = 0.0;
	}
}

void CGameTimer::recordTimeDifference(double timeDifference)
{
	// �ִ� ���� ���� �ʰ��ϸ� ���� ������ ����� ����
	if (timeDifferences.size() >= maxSamples) {
		timeDifferences.pop_front();
	}

	// ���ο� �ð� ���� ���
	timeDifferences.push_back(timeDifference);
}

double CGameTimer::calculateAverageFPS() const
{
	if (timeDifferences.empty()) {
		return 0.0; // ����� ������ FPS�� 0
	}

	double totalTime = 0.0;
	for (double time : timeDifferences) {
		totalTime += time;
	}

	// ��� �ð� ���̸� ���ϰ�, �׿� ���� FPS ��� (1 / ��� �ð�)
	double averageTime = totalTime / timeDifferences.size();
	return averageTime > 0.0 ? 1.0 / averageTime : 0.0;
}



