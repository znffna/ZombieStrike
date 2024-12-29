///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// CGameTimer.h : CGameTimer 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <deque>

class CGameTimer
{
public:
	CGameTimer();
	~CGameTimer();

	float GameTime() const;	 // 초 단위
	float DeltaTime() const; // 초 단위

	void Reset(); // 메시지 루프 이전에 호출해야 함.
	void Start(); // 타이머를 시작 또는 재개할 때 호출해야 함.
	void Stop(); // 타이머를 정지(일시 정지)할 때 호출해야 함.
	void Tick(float fLockFPS = 0.0f); // 매 프레임 호출해야 함.

	void recordTimeDifference(double timeDifference); // 시간 차이 기록
	double calculateAverageFPS() const; // 평균 FPS 계산

private:
	double m_fSecondsPerCount;
	double m_fDeltaTime;

	__int64 m_nBaseTime; // 타이머가 시작된 시간( Reset() 호출 시)
	__int64 m_nPausedTime; // 타이머가 일시 정지한 시간
	__int64 m_nStopTime; // 타이머가 정지된 시간
	__int64 m_nPrevTime; // 이전 프레임의 시간
	__int64 m_nCurrTime; // 현재 프레임의 시간

	bool m_bStopped;

	std::deque<double> timeDifferences; // 각 프레임 간 시간 차이 기록 (초 단위)
	size_t maxSamples = 50; // 최대 샘플 수
};

