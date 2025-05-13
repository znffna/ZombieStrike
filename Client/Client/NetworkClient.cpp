#include "NetworkClient.h"
#include "OnlineScene.h"
//#pragma comment (lib, "WS2_32.LIB")

//constexpr const char* LOOPBACK_IP = "127.0.0.1";

std::string SERVER_IP = "192.168.65.133";
bool g_bNetworkDebugMode = false;

NetworkingClient::NetworkingClient(COnlineScene* pScene) : recv_over(this), m_pScene(pScene) { // recv_over(this)
	// ZeroMemory(&recv_over, sizeof(recv_over));
    //ZeroMemory(recv_buffer, sizeof(recv_buffer));
    //ZeroMemory(&recv_wsabuf, sizeof(recv_wsabuf));
}

void NetworkingClient::CheckSocket()
{
    int err{};
    socklen_t len = sizeof(err);
    int ret = getsockopt(c_socket, SOL_SOCKET, SO_ERROR, (char*) & err, &len);
    if (ret == 0) {
        if (err != 0) {
			// 소켓에 오류가 발생한 경우
			std::string DebugOutput = "Socket error: " + std::to_string(err) + "\n";
			OutputDebugStringA(DebugOutput.c_str());
        }
        else {
            std::string DebugOutput = "Socket is still OK \n";
            OutputDebugStringA(DebugOutput.c_str());
        }
    }
    else {
        std::string DebugOutput = "getsockopt failed with error: ";
        OutputDebugStringA(DebugOutput.c_str());
        error_display(DebugOutput.c_str(), WSAGetLastError());
    }
}

bool NetworkingClient::Connect()
{
    WSADATA wsaData;
    auto ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

    c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (c_socket == INVALID_SOCKET) error_display("소켓 생성 실패", WSAGetLastError());

    sockaddr_in serverAddr{};
    std::string serverIP = USSING_IP;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);

    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (WSAConnect(c_socket, reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr), NULL, NULL, NULL, NULL) == SOCKET_ERROR)
    {
        error_display("서버 연결 실패", WSAGetLastError());
        // TODO : 오류가 발생하면 OnlineScene을 제외시키고 다시 Title로 가는 등의 로직 필요. 
		// TODO : 즉, OnlineScene의 Initialize가 연결 이후로 가야함.
        return false;
    }

	std::string DebugOutput = "서버 연결 성공\n";
	OutputDebugStringA(DebugOutput.c_str());

    CheckSocket();

    // 연결 시 flag 초기화
    SetConnect(true);
    // 사실상 아래 코드는 Network를 사용하는 Scene에서 호출해야함.
	// 예시 클라이언트 이름 (나중에 입력받도록 수정 가능)
    std::string name{ "Client" };

    // 1. 로그인 패킷 전송
    SendLoginPacket(name);
    return true;
}

bool NetworkingClient::StartRecvLoop()
{
	// 1. 연결 여부 확인
    if (false == IsConnect()) return false;
	// 2. recv loop이 이미 수행중인지 확인
	if (is_running) {
		std::string DebugOutput = "recv loop이 이미 수행중입니다.\n";
		OutputDebugStringA(DebugOutput.c_str());
		return false;
	}

	// 3. recv loop 시작
	is_running = true;
	recv_packet();

    return true;
}

void NetworkingClient::Logout()
{
    {
        std::string DebugOutput = "서버와의 연결 종료\n";
        OutputDebugStringA(DebugOutput.c_str());
    }
	
    SetConnect(false);
	while (true == IsRunning()) {
        SleepEx(0, TRUE); // 네트워크 I/O 콜백 처리
	}

    closesocket(c_socket);
    WSACleanup();
}

void NetworkingClient::SendLoginPacket(std::string& name)
{
    pkt_cs_login loginPkt{};
    loginPkt.header.size = sizeof(pkt_cs_login);
    loginPkt.header.type = PKT_TYPE::C_S_LOGIN;
    loginPkt.skin_type = 1;
    strcpy_s(loginPkt.name, MAX_NAME_SIZE, name.c_str());
    send_packet((char*)&loginPkt);
}

void NetworkingClient::error_display(const char* msg, int err_no)
{
    WCHAR* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);

	std::wstring DebugOutput = lpMsgBuf;
	DebugOutput += L"\n";
    OutputDebugString(DebugOutput.c_str());

    LocalFree(lpMsgBuf);
    //exit(1);
}

void NetworkingClient::recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    ExtentOverlapped* over = reinterpret_cast<ExtentOverlapped*>(p_over);

	// 수신된 바이트 수가 0이거나 에러가 발생한 경우 통신을 종료한다.
    if (err != 0 || num_bytes == 0) {

		std::string DebugOutput = "recv_callback 에러 발생 :  if (err != 0 || num_bytes == 0)\n";
		DebugOutput += "Error : " + std::to_string(err) + "\n";
		DebugOutput += "num_bytes : " + std::to_string(num_bytes) + "\n";
		OutputDebugStringA(DebugOutput.c_str());
        Logout();
        return;
    }

    // 패킷 조립
    char* recv_p = recv_over._buffer;

    DWORD offset = 0;
    DWORD remain_bytes = this->remain_bytes + num_bytes;

    while (offset < remain_bytes) {
        if (remain_bytes - offset < sizeof(PacketHeader))
        {
			// 패킷의 길이 저장 데이터(packet.size)보다 잔여 데이터가 작을 경우
            // 패킷의 길이를 모르기에 바로 종료
            if( g_bNetworkDebugMode ){
                std::string DebugOutput = "recv_callback() - 남은 바이트 수 : " + std::to_string(remain_bytes - offset) + "즉, 패킷 크기 확인 불가. 루프 종료\n";
                OutputDebugStringA(DebugOutput.c_str());
            }
            break;
        }
		PacketHeader* packet_header = (PacketHeader*)recv_p;
        DWORD size = (DWORD)packet_header->size;     // 패킷 길이

		if (remain_bytes - offset < size) {
            // 수신된 패킷이 완전하지 않은 경우
            if ( g_bNetworkDebugMode ) {
                std::string DebugOutput = "recv_callback() - 남은 바이트 수 : " + std::to_string(remain_bytes - offset) + " < 패킷의 크기(" + std::to_string(size) + ")/ 즉, 패킷 크기 부족. 루프 종료\n";
                OutputDebugStringA(DebugOutput.c_str());
            }
            break;
		}

        ProcessPacket(packet_header);

        // 사이즈 갱신
        offset += size;
        // 실제 패킷 위치 갱신
        recv_p += size;
    }

	// 패킷 처리 후 남은 바이트 수
    this->remain_bytes = remain_bytes - offset;
    // 받은 버퍼에 남은 바이트를 이동 및 초기화
    if (remain_bytes > 0) memcpy(recv_over._buffer, recv_p, this->remain_bytes);

    if (g_bNetworkDebugMode) {
        std::string DebugOutput = "패킷 처리후 남은 바이트 : "+ std::to_string(this->remain_bytes) + "\n";
        OutputDebugStringA(DebugOutput.c_str());
    }

    // 루프 종료 판단
    if (false == IsConnect()) {
        if (g_bNetworkDebugMode) {
            std::string DebugOutput = "conneect가 종료되어 Loop를 탈출함.\n";
            OutputDebugStringA(DebugOutput.c_str());
        }

        // 루프 종료됬음을 전달.
        is_running = false;
        return;
    }

    // 다시 수신 등록
    recv_packet();
}

void NetworkingClient::send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    delete p_over;
}

void NetworkingClient::ProcessPacket(PacketHeader* recv_p)
{  
	//if (is_connect == false) return; // 종료된 경우 패킷 처리하지 않음
    //if (m_pScene) m_pScene->ProcessPacket(recv_p); // Scene이 nullptr인 경우 처리하지 않음  

    if (IsConnect() == false) return;

    if (g_bNetworkDebugMode) {
        std::string debug = "[클라] ProcessPacket() 호출됨\n";
        debug += "  Packet Type: " + GetPacketName(recv_p->type) + "\n";
        debug += "  Packet Size: " + std::to_string((int)recv_p->size) + "\n";
        OutputDebugStringA(debug.c_str());
    }
    
    m_pScene->ProcessPacket(recv_p);
}

void NetworkingClient::recv_packet()
{
    ZeroMemory(&recv_over._overlapped, sizeof(recv_over._overlapped));
    recv_over._wsabuf.buf = recv_over._buffer + remain_bytes;
    recv_over._wsabuf.len = sizeof(recv_over._buffer) - remain_bytes;
    
  
	DWORD recv_flag = 0;
    int ret = WSARecv(c_socket, &recv_over._wsabuf, 1, nullptr, &recv_flag, &recv_over._overlapped, g_recv_callback);

    if (g_bNetworkDebugMode)
    {
        static int count = 0;
        {
            ++count;
            std::string DebugOutput = std::to_string(count) + " / recv_packet() - recv_over._overlapped : " + std::to_string(reinterpret_cast<uintptr_t>(&recv_over._overlapped)) + "\n";
            DebugOutput += "recv_packet() - recv_over._wsabuf.buf : " + std::to_string(reinterpret_cast<uintptr_t>(recv_over._wsabuf.buf)) + "\n";
            DebugOutput += "recv_packet() - recv_over._wsabuf.len : " + std::to_string(recv_over._wsabuf.len) + "\n";
            OutputDebugStringA(DebugOutput.c_str());
        }
    }

    if (ret == 0) {
        // 즉시 수신 완료
		DWORD num_bytes = 0;
        if (g_bNetworkDebugMode) {
            std::string debug = "즉시 수신 \n";
            OutputDebugStringA(debug.c_str());
        }
    }
    else if (ret == SOCKET_ERROR) {
        if (WSAGetLastError() == WSA_IO_PENDING) {
            // 비동기 수신 대기 중
            if (g_bNetworkDebugMode) {
                std::string debug = "비동기 수신 대기상태로 진입\n";
                OutputDebugStringA(debug.c_str());
            }
        }
        else {
            // 오류 발생
            error_display("WSARecv 실패 / Massage : ", WSAGetLastError());
            Logout();
        }
    }
}

void NetworkingClient::send_packet(char* packet) {
    ExtentOverlapped* send_over = new ExtentOverlapped{ packet, this };
    int ret = WSASend(c_socket, &send_over->_wsabuf, 1, 0, 0, &send_over->_overlapped, g_send_callback);
    if (ret == 0) {
        // 즉시 전송 완료
        // delete send_over;
    }
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        OutputDebugStringA("WSASend 실패\n");
        error_display("WSASend 실패", WSAGetLastError());
    }
}

std::string GetPacketName(PKT_TYPE packetType) {
    const std::unordered_map<PKT_TYPE, std::string> packetNames = {
        { C_S_LOGIN, "C_S_LOGIN" },
        { C_S_UPDATE, "C_S_UPDATE" },
        { C_S_SHOOT, "C_S_SHOOT" },
        { C_S_HIT, "C_S_HIT" },
        { S_C_OBJ_INFO, "S_C_OBJ_INFO" },
        { S_C_OBJECT_ADD, "S_C_OBJECT_ADD" },
        { S_C_OBJECT_UPDATE, "S_C_OBJECT_UPDATE" },
        { S_C_OBJECT_REMOVE, "S_C_OBJECT_REMOVE" },
        { S_C_STAGE_INFO, "S_C_STAGE_INFO" },
        { S_C_SCORE_INFO, "S_C_SCORE_INFO" }
    };

    return packetNames.at(packetType);
}

void g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    //ExtentOverlapped* over = reinterpret_cast<ExtentOverlapped*>(p_over);
    //NetworkingClient* client = over->_owner;
	//client->recv_callback(err, num_bytes, p_over, flag);
    NetworkingClient* client = reinterpret_cast<NetworkingClient*>(p_over);
    client->recv_callback(err, num_bytes, p_over, flag);
}

void g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    ExtentOverlapped* p_sendover = reinterpret_cast<ExtentOverlapped*>(p_over);
    delete p_sendover;
}
