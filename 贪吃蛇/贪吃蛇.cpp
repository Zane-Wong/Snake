// 贪吃蛇.cpp
#include <iostream>
#include <vector>
#include <Windows.h>
#include <string.h>
#include <ctime>
#include <conio.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#define MAP_WIDTH 50;
#define MAP_HEIGHT 25;
int height, width;//窗口宽高
int food_x, food_y;//食物坐标
int head_x, head_y;//蛇头坐标
int next_x, next_y;//蛇头下一步坐标
int score;//得分
int delta_time;//时间间隔
bool canCross;//能否穿墙
bool invincible = false;//无敌模式，修改为true时启用
HANDLE hDirectionMutex;//方向句柄
enum BlockType {
	Blank = ' ', //空白
	SnakeBody = 'O',//蛇身
	SnakeHead = '@',//蛇头
	Food = '*'//食物
};
enum Direction { //蛇移动方向
	Up,
	Down,
	Left,
	Right
};
class Block {
private:
	unsigned short life;
	BlockType type;
public:
	Block() { life = 0; type = BlockType::Blank; }
	~Block() {}
	void setLife(int life){
		if (life >= 0)this->life = (unsigned short)life;
		if (life == 0) type = BlockType::Blank;
	}
	unsigned short getLife() { return life; }
	void lifeUp() {
		life++;
	}
	void lifeDown() {
		if (life > 0)life--;
		if (life == 0)type = BlockType::Blank;
	}
	void setType(BlockType type) {
		this->type = type;
	}
	char getType() {
		return (char) type;
	}
};
std::vector<std::vector<Block>>map;//游戏地图
Direction direction;//移动方向
void moveCursor(int x, int y);//移动光标
void hideCursor(void);//隐藏光标
void init();//初始化设置
void initData();//初始化数据
void createFood();//创建食物
void eatFood();//吃食物
void crossWall();//穿墙
bool checkFail(int next_x, int next_y);//检测失败
void paint();//打印
void nextStep();//单步走
void changeDirection(Direction);//修改蛇移动方向
DWORD WINAPI move(LPVOID pram);//蛇移动
DWORD WINAPI dealOp(LPVOID pram);//响应玩家操作
int main()
{
	char op;
	char prompt[] = "Press anykey to start.\n按任意键开始游戏";
	init();
	hideCursor();
	//std::cout << "是否进入游戏？(y/n)" << std::endl;
	HANDLE moveThread;
	//while((op=_getch())!='n'){
		initData();
		char mode;
		std::cout << "请选择游戏模式：" << "----> 1.不可穿墙  2.可穿墙 <----" << std::endl;
		mode = _getch();
		while (mode != 49 && mode != 50) {
			mode = _getch();
		}
		if (mode == 50) canCross = true;
		paint();
		if(!invincible){
			std::cout << prompt;
			if(_getch()) {
				system("cls");
				moveThread = CreateThread(0, 0, move, 0, 0, 0);//线程
			}
		}
		dealOp(NULL);//线程2 map互斥修改
		//std::cout << "再来一局？(y/n)\n";
	//}
	//return 0;
}
void moveCursor(int x,int y) {
	COORD pos = { x,y };
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hOut, pos);
}
void hideCursor(void) {
	CONSOLE_CURSOR_INFO cursor_info = { 1, 0 };
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}
void initData() {
	delta_time = 500;
	score = 0;
	direction = Direction::Right;
	canCross = false;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			map[i][j].setLife(0);
			map[i][j].setType(BlockType::Blank);
		}
	}
	head_x = (width - 1) / 2;
	head_y = (height - 1) / 2;
	next_x = head_x;
	next_y = head_y;
	map[head_y][head_x].setLife(score + 1);
	map[head_y][head_x].setType(BlockType::SnakeHead);
	createFood();
}
void init() {
	system("color 70");	
	height = MAP_HEIGHT;
	width = MAP_WIDTH;
	char setWindSize[] = "mode con cols=%d lines=%d";
	sprintf_s(setWindSize, setWindSize, width+2, height+5);
	system(setWindSize);
	hDirectionMutex = CreateMutex(0, 0, L"Direction");//方向
	for (int i = 0; i < height; i++) {
		std::vector<Block> tmp;
		for (int j = 0; j < width; j++) {
			tmp.push_back(Block());
		}
		map.push_back(tmp);
	}
	char title[] = ">>>>>>>>>贪吃蛇<<<<<<<<<";
	std::string op1("--------操作方式--------");
	std::string op2("W、A、S、D 控制方向，长按加速；P 暂停，Q 退出。");
	std::cout << std::string((width - strlen(title)) / 2, ' ') << title << std::endl << std::endl
		<< std::string((width - strlen("--------图形说明--------")) / 2, ' ') << "--------图形说明--------" << std::endl
		<< std::string((width - strlen("@：蛇头 O：蛇身 * ：食物")) / 2, ' ') << "@：蛇头 O：蛇身 * ：食物" << std::endl << std::endl
		<< std::string((width - op1.size()) / 2, ' ') << op1 << std::endl << std::string((width - op2.size()) / 2, ' ') << op2 << std::endl << std::endl
		//<< "游戏规则：每吃掉一次食物，蛇身就增加一节，同时增加积分;随着分数的增加，蛇身移动速度会越来越快。" << std::endl
		<< std::string((width - strlen("--------游戏模式--------")) / 2, ' ') << "--------游戏模式--------" << std::endl
		<< "本游戏包含两种模式,可穿墙和不可穿墙，可穿墙模式下蛇头碰到边界不会结束游戏，不可穿墙模式下蛇头碰到边界则游戏结束，无论哪个模式,蛇头碰到自己的身体都会结束游戏。" 
		<< std::endl << std::endl;
}
void createFood() {
	unsigned seed = time(0);
	srand(seed);
	food_x = rand() % MAP_WIDTH;
	food_y = rand() % MAP_HEIGHT;
	while (map[food_y][food_x].getLife()!=0) {
		food_x = rand() % MAP_WIDTH;
		food_y = rand() % MAP_HEIGHT;
	}
	//std::cout << food_x << ',' << food_y << std::endl;
	map[food_y][food_x].setLife(1);
	map[food_y][food_x].setType(BlockType::Food);
}
void crossWall() {
	if (next_x < 0)next_x = width - 1;
	if (next_y < 0)next_y = height - 1;
	if (next_x >= width) next_x = 0;
	if (next_y >= height)next_y = 0;
}
bool checkFail(int next_x,int next_y) {
	if (next_x >= 0 && next_x < width && next_y >= 0 && next_y < height) {
		if (map[next_y][next_x].getType() == BlockType::SnakeBody)
			return true;
		else return false;
	}
	else {
		if(!canCross){
			return true; //不可穿墙
		}else{
			crossWall();
		return false;//可穿墙
		}
		
	}
}
void eatFood() {
	map[food_y][food_x].lifeDown();
	score++;
	delta_time -= 10;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (map[i][j].getType() == BlockType::SnakeBody) {
				map[i][j].lifeUp();
			}
		}
	}
	std::cout << "score: " << score;
}
void paint() {
	moveCursor(0, 0);
	std::string border(width, '-');
	std::cout << '+' << border <<'+' << std::endl;
	for (int i = 0; i < height; i++) {
		std::cout<<'|';
		for (int j = 0; j < width; j++) {
			if (map[i][j].getLife() > 0 && map[i][j].getType() == BlockType::SnakeBody) {
				map[i][j].lifeDown();
			}
			std::cout << map.at(i).at(j).getType();
		}
		std::cout << '|' << std::endl;
	}
	std::cout  << '+'<<border<<'+'<<std::endl;

} 
void nextStep() {
	map[head_y][head_x].setType(BlockType::SnakeBody);
	head_x = next_x;
	head_y = next_y;
	if (map[head_y][head_x].getType() == BlockType::Food) {
		eatFood();
		mciSendString(L"play eat.mp3",0,0,0);
		createFood();
	}
	map[head_y][head_x].setLife(score + 1);
	map[head_y][head_x].setType(BlockType::SnakeHead);
}
DWORD WINAPI move(LPVOID pram) {
	while (true) {	
		Sleep(delta_time);
		WaitForSingleObject(hDirectionMutex, INFINITE);
		switch (direction) {
		case Direction::Up:
			next_y  -=1;
			break;
		case Direction::Down:
			next_y  += 1;
			break;
		case Direction::Left:
			next_x  -= 1;
			break;
		case Direction::Right:
			next_x +=  1;
			break;
		}
		if (checkFail(next_x, next_y)) {
			mciSendString(L"play end.mp3", 0, 0, 0);
			system("cls");
			std::cout << "游戏结束！你的得分为" << score << std::endl;
			break;
		}
		nextStep();
		paint();		
		ReleaseMutex(hDirectionMutex);
	}
	return NULL;
}
void changeDirection(Direction next_direction) {
	direction = next_direction; 
}
DWORD WINAPI dealOp(LPVOID pram) {
	while (true) {
		if (_kbhit()) {
			WaitForSingleObject(hDirectionMutex, INFINITE);
			switch (_getch()) {
			case 'W': //上
			case 'w':
				if (direction != Direction::Down) {
					changeDirection(Direction::Up);
					next_y -= 1;
				}
				break;
			case 'S': //下
			case 's':
				if (direction != Direction::Up) {
					changeDirection(Direction::Down);
					next_y += 1;
				}
				break;
			case 'A': //左
			case 'a':
				if (direction != Direction::Right) {
					changeDirection(Direction::Left);
					next_x -= 1;
				}
				break;
			case 'D': //右
			case 'd':
				if (direction != Direction::Left){
					changeDirection(Direction::Right);
					next_x += 1;
				}
				break;
			case 'P': //暂停
			case 'p':
				system("pause");
				system("cls");
				break;
			case 'q': //退出
			case 'Q':
				return NULL;
			}
			if (checkFail(next_x, next_y)) {
				if (!invincible) {
					mciSendString(L"play end.mp3", 0, 0, 0);
					system("cls");
					std::cout << "游戏结束！你的得分为" << score << std::endl;
					break;
				}
			}
			nextStep();
			paint();
			ReleaseMutex(hDirectionMutex);
		}
	}
	return NULL;
}