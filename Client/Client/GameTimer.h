///////////////////////////////////////////////////////////////////////////////
// Date: 2024-12-29
// CGameTimer.h : CGameTimer Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <deque>

class CGameTimer
{
public:
	CGameTimer();
	~CGameTimer();

	float GameTime() const;	 // �� ����
	float DeltaTime() const; // �� ����

	void Reset(); // �޽��� ���� ������ ȣ���ؾ� ��.
	void Start(); // Ÿ�̸Ӹ� ���� �Ǵ� �簳�� �� ȣ���ؾ� ��.
	void Stop(); // Ÿ�̸Ӹ� ����(�Ͻ� ����)�� �� ȣ���ؾ� ��.
	void Tick(float fLockFPS = 0.0f); // �� ������ ȣ���ؾ� ��.

	void recordTimeDifference(double timeDifference); // �ð� ���� ���
	double calculateAverageFPS() const; // ��� FPS ���

private:
	double m_fSecondsPerCount;
	double m_fDeltaTime;

	__int64 m_nBaseTime; // Ÿ�̸Ӱ� ���۵� �ð�( Reset() ȣ�� ��)
	__int64 m_nPausedTime; // Ÿ�̸Ӱ� �Ͻ� ������ �ð�
	__int64 m_nStopTime; // Ÿ�̸Ӱ� ������ �ð�
	__int64 m_nPrevTime; // ���� �������� �ð�
	__int64 m_nCurrTime; // ���� �������� �ð�

	bool m_bStopped;

	std::deque<double> timeDifferences; // �� ������ �� �ð� ���� ��� (�� ����)
	size_t maxSamples = 50; // �ִ� ���� ��
};

