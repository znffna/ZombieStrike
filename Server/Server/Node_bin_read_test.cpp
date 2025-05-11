#include <iostream>
#include <fstream>
#include <windows.h>

const int MAP_SIZE = 512;


// 실제 표시할 사각형 영역 (ex. 중앙 500x500)
const int highlightStartX = 262;
const int highlightStartY = 262;
const int highlightWidth = 300;
const int highlightHeight = 300;


const int TEXTURE_SIZE = 512;
const int CELL_MAP_WIDTH = 250;
const int CELL_MAP_HEIGHT = 250;
bool walkable[TEXTURE_SIZE][TEXTURE_SIZE];

void LoadObstacleMask(const char* filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::cerr << "[ERROR] 파일 열기 실패: " << filepath << "\n";
        return;
    }

    const int BIN_WIDTH = 512;

    for (int z = 0; z < CELL_MAP_HEIGHT; ++z)
    {
        for (int x = 0; x < CELL_MAP_WIDTH; ++x)
        {
            // 좌상단 픽셀 하나만 대표값으로
            int px = x * 2;
            int py = z * 2;
            file.seekg(py * BIN_WIDTH + px, std::ios::beg);

            char byte;
            file.read(&byte, 1);
            walkable[z][x] = (byte == 0);  // 0 = 길, 1 = 장애물
        }

    }

    //for (int y = 0; y < MAP_SIZE; ++y)
    //{
    //    for (int x = 0; x < MAP_SIZE; ++x)
    //    {
    //        char byte;
    //        file.read(&byte, 1);
    //        if (file.eof())
    //        {
    //            std::cerr << "[ERROR] 파일 끝에 도달했습니다. 크기가 너무 작습니다.\n";
    //            return;
    //        }

    //        walkable[y][x] = (byte == 0); // 0이면 길, 1이면 장애물
    //    }
    //}

    std::cout << "[OK] obstacle_mask.bin 로드 완료\n";
}

void LoadObstacleMask2(const char* filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::cerr << "[ERROR] 파일 열기 실패: " << filepath << "\n";
        return;
    }

    // 파일 크기 확인
    file.seekg(0, std::ios::end);
    int fileSize = static_cast<int>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < MAP_SIZE * MAP_SIZE)
    {
        std::cerr << "[ERROR] 파일 크기 부족 (" << fileSize << " bytes)\n";
        return;
    }

    for (int y = 0; y < MAP_SIZE; ++y)
    {
        for (int x = 0; x < MAP_SIZE; ++x)
        {
            char value;
            file.read(&value, 1);
            walkable[y][x] = (value == 0);  // 0이면 길, 나머지는 장애물
        }
    }

    std::cout << "[OK] 512x512 obstacle map 로드 완료\n";
}

void LoadObstacleMask3(const char* filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::cerr << "[ERROR] 파일 열기 실패: " << filepath << "\n";
        return;
    }

    for (int z = 0; z < CELL_MAP_HEIGHT; ++z)
    {
        for (int x = 0; x < CELL_MAP_WIDTH; ++x)
        {
            int px = x * 2;
            int pz = z * 2;
            int offset = pz * TEXTURE_SIZE + px;

            file.seekg(offset, std::ios::beg);
            char value;
            file.read(&value, 1);

            walkable[z][x] = (value == 0); // 0이면 길, 1이면 장애물
        }
    }

    std::cout << "[OK] obstacle_mask.bin 로드 완료 (250x250 셀)\n";
}



void PrintMapWithBox(int centerX, int centerY, int boxWidth, int boxHeight)
{
    const int MAP_SIZE = 1024;

    const int consoleW = 92;
    const int consoleH = 55;

    int boxLeft = centerX - boxWidth / 2;
    int boxTop = centerY - boxHeight / 2;
    int boxRight = centerX + boxWidth / 2;
    int boxBottom = centerY + boxHeight / 2;

    for (int y = 0; y < consoleH; ++y)
    {
        for (int x = 0; x < consoleW; ++x)
        {
            int mapX = x * MAP_SIZE / consoleW;
            int mapY = y * MAP_SIZE / consoleH;

            bool inBox = (mapX >= boxLeft && mapX < boxRight &&
                mapY >= boxTop && mapY < boxBottom);

            std::cout << (inBox ? 'X' : '.');
        }
        std::cout << '\n';
    }
}

void PrintMiniMap2(int startY, int startX, int height, int width)
{
    for (int y = startY; y < startY + height && y < MAP_SIZE; ++y)
    {
        for (int x = startX; x < startX + width && x < MAP_SIZE; ++x)
        {
            std::cout << (walkable[y][x] ? '0' : ' '); // 0 = 길, 공백 = 장애물
        }
        std::cout << '\n';
    }
}

void PrintMiniMap3(int startY, int startX, int height, int width)
{
    for (int y = startY; y < startY + height && y < CELL_MAP_HEIGHT; ++y)
    {
        for (int x = startX; x < startX + width && x < CELL_MAP_WIDTH; ++x)
        {
            std::cout << (walkable[y][x] ? ' ' : '0');  // ' ' = 길, '0' = 장애물
        }
        std::cout << '\n';
    }
}

void PrintMiniMapSampled(int startY, int startX, int height, int width, int sample = 2)
{
    for (int y = startY; y < startY + height; y += sample)
    {
        for (int x = startX; x < startX + width; x += sample)
        {
            std::cout << (walkable[y][x] ? ' ' : '0');
        }
        std::cout << '\n';
    }
}

int main()
{
    LoadObstacleMask2("C:/GitHub/TurboBlaze/Map/Node/ob_mask_te_2.bin");

    // 중심 125,125는 250x250 맵의 정중앙
   // PrintMapWithBox(125, 125, 50, 50);       // 시야 표시
    PrintMiniMap2(0, 0, 512, 512);
    return 0;
}
