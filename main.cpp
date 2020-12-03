/*
    Autor:          Konrad Janiak 2020
    Kompilacja:     g++ main.cpp -o main -O3 -lpthread
    Kompilacja:     make c
    Uruchomienie:   ./main
    K & U:          g++ main.cpp -o main -O3 -lpthread && ./main && eog maze_img.ppm
    K & U:          make cr

    1)  Wartość MAZE_SIZE musi być równa ilości znaków w wierszu pliku z labiryntem.
    2)  Liczba w nazwie pliku txt z labiryntem oznacza ilość znaków w wierszu.
    2)  Labirynt w pliku txt powinien składać się z jedynek (ściany) i spacji (korytarz).
        Labirynt nie musi być zamknięty, ponieważ dookoła niego i tak wstawiane są ściany.
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

const int MAZE_SIZE = 21;
const char *FILE_NAME = "maze21.txt";

// TRUE = pokazuje jak rozprzestrzeniają się wątki (w kolejności: czerwony, zielony, niebieski)
// FALSE = wątek przyjmuje jeden z sześciu kolorów
// Działa dobrze dla dużych labiryntów (szerokość tablicy większa niż 30).
const bool enableSpreadColors = false;

const char LINE_FEED = 10;
const char ZERO = 48;
const char ONE = 49;
const char SPACE = 32;
const char WALL = -1;
const char CORRIDOR = 0;

int maze[MAZE_SIZE][MAZE_SIZE];
std::mutex maze_mutex[MAZE_SIZE][MAZE_SIZE];
std::mutex threadIdIncrement;
char maze_img[MAZE_SIZE][MAZE_SIZE][3];
int threadId = 1;
int rowCount = 0;

void exploreMaze(int row, int col, bool newThread, int thId);
void markCell(int row, int col, int state);

void getMazeFromFile();
void processChar(const char c, int *row, int *col);
void saveImageToFile();

bool checkLeft(const int row, const int col);
bool checkRight(const int row, const int col);
bool checkUp(const int row, const int col);
bool checkDown(const int row, const int col);
bool isCellFree(const int row, const int col);

void paintMazeBorder();
void paintCell(const int row, const int col, const int red, const int green, const int blue);
void paintCell(const int row, const int col, const int currentThreadId);

int main()
{
    getMazeFromFile();
    exploreMaze(1, 1, true, threadId);
    saveImageToFile();
    std::cout << "Threads used: " << threadId << "\n";
    return 0;
}

void exploreMaze(int row, int col, bool newThread, int thId)
{
    int thisThreadId = thId;
    bool cont = false;
    bool contUp = false;
    bool contRight = false;
    bool contDown = false;
    bool contLeft = false;
    bool newThreadUp = false;
    bool newThreadRight = false;
    bool newThreadDown = false;
    bool newThreadLeft = false;
    std::vector<std::thread> threads;

    maze_mutex[row][col].lock();
    {
        if (newThread)
        {
            threadIdIncrement.lock();
            {
                threadId++;
                thisThreadId = threadId;
            }
            threadIdIncrement.unlock();
        }
        markCell(row, col, thisThreadId);
    }

    maze_mutex[row][col].unlock();

    paintCell(row, col, thisThreadId);

    if (checkDown(row, col))
    {
        if (!cont)
        {
            cont = true;
            contDown = true;
        }
    }

    if (checkRight(row, col))
    {
        if (!cont)
        {
            cont = true;
            contRight = true;
        }
        else
            newThreadRight = true;
    }

    if (checkUp(row, col))
    {
        if (!cont)
        {
            cont = true;
            contUp = true;
        }
        else
            newThreadUp = true;
    }

    if (checkLeft(row, col))
    {
        if (!cont)
        {
            cont = true;
            contLeft = true;
        }
        else
            newThreadLeft = true;
    }

    if (newThreadRight)
        threads.push_back(std::thread(exploreMaze, row, col + 1, true, thisThreadId));

    if (newThreadUp)
        threads.push_back(std::thread(exploreMaze, row - 1, col, true, thisThreadId));

    if (newThreadLeft)
        threads.push_back(std::thread(exploreMaze, row, col - 1, true, thisThreadId));

    if (cont)
    {
        if (contUp)
            exploreMaze(row - 1, col, false, thisThreadId);

        if (contRight)
            exploreMaze(row, col + 1, false, thisThreadId);

        if (contDown)
            exploreMaze(row + 1, col, false, thisThreadId);

        if (contLeft)
            exploreMaze(row, col - 1, false, thisThreadId);
    }

    for (int i = 0; i < threads.size(); i++)
        threads[i].join();
}

void getMazeFromFile()
{
    std::ifstream mazeFile;
    mazeFile.open(FILE_NAME);
    if (!mazeFile.is_open())
    {
        std::cout << "Plik nie zostal otwarty\n";
        exit(EXIT_FAILURE);
    }

    char c;
    int row = 0;
    int col = 0;
    mazeFile.get(c);
    while (mazeFile.good())
    {
        processChar(c, &row, &col);
        mazeFile.get(c);
    }
    mazeFile.close();

    paintMazeBorder();
}

void processChar(const char c, int *row, int *col)
{
    if (*row == 0)
        rowCount++;

    if (c == ONE)
    {
        markCell(*row, *col, WALL);
        paintCell(*row, *col, 0, 0, 0);
    }
    else if (c == SPACE)
    {
        markCell(*row, *col, CORRIDOR);
        paintCell(*row, *col, 255, 255, 255);
    }
    else if (c == LINE_FEED)
    {
        (*row)++;
        *col = 0;
        return;
    }
    (*col)++;
}

void markCell(int row, int col, int state) { maze[row][col] = state; }

bool checkLeft(const int row, const int col) { return isCellFree(row, col - 1); }

bool checkRight(const int row, const int col) { return isCellFree(row, col + 1); }

bool checkUp(const int row, const int col) { return isCellFree(row - 1, col); }

bool checkDown(const int row, const int col) { return isCellFree(row + 1, col); }

bool isCellFree(const int row, const int col) { return maze[row][col] == 0; }

void paintCell(const int row, const int col, const int red, const int green, const int blue)
{
    maze_img[row][col][0] = red;
    maze_img[row][col][1] = green;
    maze_img[row][col][2] = blue;
}

void paintCell(const int row, const int col, const int currentThreadId)
{
    if (enableSpreadColors)
    {
        int currMod = currentThreadId % 255;
        if (currentThreadId < 255)
            paintCell(row, col, currMod, 0, 0);
        else if (currentThreadId < 2 * 255)
            paintCell(row, col, 0, currMod, 0);
        else
            paintCell(row, col, 0, 0, currMod);
    }
    else
    {
        int numOfColors = 6;
        if (currentThreadId % numOfColors == 0)
            paintCell(row, col, 0, 0, 255);
        else if (currentThreadId % numOfColors == 1)
            paintCell(row, col, 0, 255, 0);
        else if (currentThreadId % numOfColors == 2)
            paintCell(row, col, 0, 255, 255);
        else if (currentThreadId % numOfColors == 3)
            paintCell(row, col, 255, 0, 0);
        else if (currentThreadId % numOfColors == 4)
            paintCell(row, col, 255, 0, 255);
        else if (currentThreadId % numOfColors == 5)
            paintCell(row, col, 255, 255, 0);
    }
}

void paintMazeBorder()
{
    for (int i = 0; i < rowCount; i++)
    {
        for (int j = 0; j < rowCount; j++)
        {
            if ((i == 0) ||
                (j == 0) ||
                (j == rowCount - 2) ||
                (i == rowCount - 2))
            {
                maze[i][j] = -1;
                paintCell(i, j, 255, 255, 255);
            }
        }
    }
}

void saveImageToFile()
{
    FILE *fp = fopen("maze_img.ppm", "wb");
    fprintf(fp, "P6\n%i %i\n%i ", MAZE_SIZE, MAZE_SIZE, 255);
    fwrite(maze_img, 1, MAZE_SIZE * MAZE_SIZE * 3, fp);
    fclose(fp);
}