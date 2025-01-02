// Server�� Client ���� ����� ���� �������� ����
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

// ������ �ִ� ���� ��
constexpr int MAX_USER = 5000;

// ���� ũ�� ����
constexpr int W_WIDTH = 400;
constexpr int W_HEIGHT = 400;

// Packet ID ���� (Packet ID Definition)
// ex) constexpr int PKT_CS_LOGIN = 1;
// ex) enum PKT_CS { PKT_CS_LOGIN = 1, PKT_CS_CHAT = 2, ... };
// ex) enum PKT_SC { PKT_SC_LOGIN_OK = 1, PKT_SC_LOGIN_FAIL = 2, ... };


#pragma pack (push, 1)

// ��Ŷ ���� (Packet Definition)
// ex) struct PKT_CS_LOGIN { int id; char name[NAME_SIZE]; };

#pragma pack (pop)