// Server와 Client 간의 통신을 위한 프로토콜 정의
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

// 서버의 최대 세션 수
constexpr int MAX_USER = 5000;

// 맵의 크기 정의
constexpr int W_WIDTH = 400;
constexpr int W_HEIGHT = 400;

// Packet ID 정의 (Packet ID Definition)
// ex) constexpr int PKT_CS_LOGIN = 1;
// ex) enum PKT_CS { PKT_CS_LOGIN = 1, PKT_CS_CHAT = 2, ... };
// ex) enum PKT_SC { PKT_SC_LOGIN_OK = 1, PKT_SC_LOGIN_FAIL = 2, ... };


#pragma pack (push, 1)

// 패킷 정의 (Packet Definition)
// ex) struct PKT_CS_LOGIN { int id; char name[NAME_SIZE]; };

#pragma pack (pop)