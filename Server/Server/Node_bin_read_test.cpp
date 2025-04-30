#include <iostream>
#include <fstream>
#include <windows.h>

const int MAP_SIZE = 1024;


// 실제 표시할 사각형 영역 (ex. 중앙 500x500)
const int highlightStartX = 262;
const int highlightStartY = 262;
const int highlightWidth = 300;
const int highlightHeight = 300;

bool walkable[MAP_SIZE][MAP_SIZE];

void LoadObstacleMask(const char* filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::cerr << "[ERROR] 파일 열기 실패: " << filepath << "\n";
        return;
    }

    for (int y = 0; y < MAP_SIZE; ++y)
    {
        for (int x = 0; x < MAP_SIZE; ++x)
        {
            char byte;
            file.read(&byte, 1);
            if (file.eof())
            {
                std::cerr << "[ERROR] 파일 끝에 도달했습니다. 크기가 너무 작습니다.\n";
                return;
            }

            walkable[y][x] = (byte == 0); // 0이면 길, 1이면 장애물
        }
    }

    std::cout << "[OK] obstacle_mask.bin 로드 완료\n";
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



void PrintMiniMap(int startY, int startX, int height, int width)
{
    for (int y = startY; y < startY + height; ++y)
    {
        for (int x = startX; x < startX + width; ++x)
        {
            std::cout << (walkable[y][x] ? ' ' : '0'); // 0 = 장애물, ' ' = 길
        }
        std::cout << '\n';
    }
}


int main()
{
    LoadObstacleMask("C:/GitHub/TurboBlaze/Map/Node/obstacle_mask.bin");
    PrintMapWithBox(512, 512, 100, 200);
    PrintMiniMap(412, 412, 200, 200);

    return 0;
}
