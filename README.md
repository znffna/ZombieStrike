# TurboBlaze
 
## Client

### TODO 리스트

| 진행 상황 | TODO 목록 | 마감일 | 비고 |
|:-------|:-------|:-------|:-------|
| 완료 | 기초 프레임워크 구조 작성 | 1월 1주차 |
| 완료 | Scene 및 Transform 작성 | 1월 4주차 |
| 완료 | 기초 Shader, CubeMesh 작성 | 2월 1주차 |
| 완료 | Texture 및 Scene과 Object 연결 | 2월 2주차 |
| 완료 | Camera 구성 및 | 2월 3주차 |
| 완료 | Terrain, Skybox 작성 | 2월 4주차 |
| 완료 | 임시 카메라 회전 코드 작성 | 3월 1주차 | 플레이어를 추적하는 로직 필요
| 완료 | Zombie Animation 로직 작성 | 3월 2주차 |
| 완료 | RigidBody 작성 및 적용 | 3월 3주차 |
| 완료 | 모델 텍스쳐 png화 및 Tansform 분리  | 3월 4주차 |
| 완료 | 리소스 풀 작성 | 4월 1주차 |
| 완료 | 맵 추출 및 Load 작성 | 4월 2주차 | 맵의 데이터에 Collider 정보 추출 필요
| 완료 | Collider 작성 및 출력 | 4월 3주차 | Collider를 다루는 별도 Container 사용 여부 결정 필요
| 완료 | Model의 meshBound 바탕 Collider Merge | 4월 4주차 | 해당 요소로 Shadow Map 생성 로직 필요
| 진행중 | Network I/O to Client | 4월 5주차 |
| 시작전 | 총 및 총알 작성 |  |
| 시작전 | HP class 작성 및 적용 |  |
| 시작전 | Scene Change |  |
| 시작전 | Shadow 렌더링 작성 |  |
| 시작전 | 사격 피드백 적용 |  |


## Server

### TODO 리스트

| 진행 상황 | TODO 목록 | 마감일 | 비고 |
|:-------|:-------|:-------|:-------|
| 완료 |  게임에서 사용할 맵, 텍스처, 총기 및 케릭터 모델등 탐색 | 1월 1주차 |
| 완료 |  Server 기본 솔루션 구성 및 IOCP 구조 설계 | 1월 4주차 | 이후 TCP Overlapped 모델로 전환 예정
| 완료 |  유니티에서 맵 구성 및 맵 추출 완료 | 2월 2주차 | 
| 완료 |  TCP+Overlapped 서버 구조 구현 | 3월 3주차 | EXP_OVER, SESSION 구조 사용
| 완료 |  Protocol 및 Packet 구조 정의 | 3월 4주차 | 고정 헤더 구조로 통일
| 완료 |  Player Session 구조 정비 | 4월 1주차 | 클라이언트 연결/해제 처리
| 완료 |  충돌 관련 패킷 정의 및 전달 구조 완성 | 4월 2주차 | 클라이언트에서 보낸 충돌 정보 수신 처리
| 완료 | Collider 병합 및 충돌 정리 문서 작성 | 4월 3주차 | Model Bound 기반 충돌 처리 설계
| 완료 |  불필요한 IOCP 제거 및 서버 구조 리팩토링 | 4월 4주차 | TCP 기반으로 재정비 완료
| 진행중 |  Network I/O to Client 구조 개선, 맵 구조 추출 | 4월 5주차 | Broadcast 성능 개선 및 Dirty Flag 적용 예정
| 시작전 |  A* 기반 좀비 경로 탐색 구조 구현 | 5월 1주차 | 맵 기반 Tile 구조 필요
| 시작전 |  ThreadPool + A* 병렬 처리 구조 설계 | 5월 2주차 | 최대 1000마리 좀비 동시 처리
| 시작전 | FSM 기반 좀비 AI 설계 (Idle, Move, Chase) | 5월 3주차 | 상태 전이 조건 정리 필요
| 시작전 |  좀비 갯수 증가에 대한 최적화 대응 | 5월 4주차 | Object 풀링 및 동적 갱신 처리
| 시작전 |  총기 피격 처리 및 사망 패킷 처리 | 6월 1주차 | Ray-Sphere 기반 판정 또는 Hitbox 설계
| 시작전 |  Wave 제어 시스템 구조화 | 6월 2주차 | 동시 생성 제어 및 시점 처리
| 시작전 |  아이템 드롭, 획득 로직 구현 | 6월 3주차 | 위치/ID 기반 송수신 처리 필요
| 시작전 |  서버 성능 로그 및 테스트 기능 작성 | 6월 4주차 | FPS 측정, RTT 체크 포함

