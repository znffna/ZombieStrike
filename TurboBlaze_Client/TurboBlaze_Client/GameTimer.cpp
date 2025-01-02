///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// CGameTimer.cpp : CGameTimer 클래스의 구현 파일
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

// Reset()이 호출된 이후 경과한 총 시간을 반환한다.
// 시간이 정지되어 있는 동안의 시간은 포함하지 않는다.
float CGameTimer::GameTime() const
{
	// 타이머가 정지 상태이면, 정지된 시점부터 흐른 시간은 계산하지 말아야 한다.
	// 또한, 이전에 이미 일시 정지된 적이 있다면 시간차 m_nStopTime - m_nBaseTime에는
	// 일시 정지 누적 시간이 포함되어 있는데, 그 누적 시간은 전체 시간에 포함하지 
	// 말아야 한다. 이를 바로잡기 위해, m_nStopTime에서 일시 정지 누적 시간을
	// 뺀다.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if (m_bStopped)
	{
		return (float)(((m_nStopTime - m_nPausedTime)
			- m_nBaseTime) * m_fSecondsPerCount);
	}

	// 시간차 m_nCurrTime - m_nBaseTime에는 일시 정지 누적 시간이 포함되어 있다. 이를
	// 전체 시간에 포함하면 안 되므로, 그 시간을 m_CurrTime에서 빼준다.
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


	// 정지(일시 정지)와 시작(재개) 사이에 흐른 시간을 누적한다.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// 정지 상태에서 타이머를 재개하는 경우 :
	if (m_bStopped)
	{
		// 일시 정지된 시간을 누적한다.
		m_nPausedTime += (startTime - m_nStopTime);

		// 타이머를 닷 ㅣ시작하는 것이므로, 현재의 m_nPrevTime(이전 시간)은
		// 유효하지 않다(일시 정지 도중에 갱신되었을 것이므로).
		// 따라서 현재 시간으로 다시 설정한다.
		m_nPrevTime = startTime;

		// 이제는 정지 상태가 아니므로 관련 멤버들을 갱신한다.
		m_nStopTime = 0;
		m_bStopped = false;
	}
}

void CGameTimer::Stop()
{
	// 이미 정지 상태이면 아무 일도 하지 않는다.
	if (!m_bStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		// 그렇지 않다면 현재 시간을 타이머 정지 시점 시간으로 저장하고,
		// 타이머를 정지되었음을 뜻하는 부울 플래그를 설정한다.
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

	// 이번 프레임의 시간을 얻는다..
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_nCurrTime = currTime;

	// 이번 프레임의 시간과 이전 프레임의 시간의 차이를 구한다.
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

	// 다음 프레임을 준비한다.
	m_nPrevTime = m_nCurrTime;

	// 음수가 되지 않게 한다.  SDK 문서화의 CDXUTTimer 항목에 따르면,
	// 프로세서가 절전 모드로 들어가거나 실행이 다른 프로세서와
	// 엉키는 경우  mDeltaTime이 음수가 될 수 있다.
	if (m_fDeltaTime < 0.0)
	{
		m_fDeltaTime = 0.0;
	}
}

void CGameTimer::recordTimeDifference(double timeDifference)
{
	// 최대 샘플 수를 초과하면 가장 오래된 기록을 삭제
	if (timeDifferences.size() >= maxSamples) {
		timeDifferences.pop_front();
	}

	// 새로운 시간 차이 기록
	timeDifferences.push_back(timeDifference);
}

double CGameTimer::calculateAverageFPS() const
{
	if (timeDifferences.empty()) {
		return 0.0; // 기록이 없으면 FPS는 0
	}

	double totalTime = 0.0;
	for (double time : timeDifferences) {
		totalTime += time;
	}

	// 평균 시간 차이를 구하고, 그에 대한 FPS 계산 (1 / 평균 시간)
	double averageTime = totalTime / timeDifferences.size();
	return averageTime > 0.0 ? 1.0 / averageTime : 0.0;
}



