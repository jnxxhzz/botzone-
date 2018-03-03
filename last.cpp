/**
* Tetris2 样例程序
* 20181031更新：将elimBonus[count]改为elimBonus[count - hasBonus]。
* 20181027更新：将trans数组的第二维长度从4加大到6，感谢kczno1用户的指正。
* https://wiki.botzone.org/index.php?title=Tetris2
*/
// 注意：x的范围是1~MAPWIDTH，y的范围是1~MAPHEIGHT
// 数组是先行（y）后列（c）
// 坐标系：原点在左下角

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <queue>
//#include "jsoncpp/json.h" 
using namespace std;

#define MAPWIDTH 10
#define MAPHEIGHT 20


// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
double R;
int currBotColor;
int enemyColor;
int turnID, blockType;
int blockForEnemy, finalX, finalY, finalO;

int Strategy;
int FINDWIDTH = 4;//搜索宽度 
int old;//棋盘记录 
int MINHEIGHT;//非对手给的最低高度 
int minCount[3]={0};
int maxCount[3]={0};
int BESTHIGHT[MAPWIDTH + 5]={0};

int nextTypeForColor[2];
// 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界
// （2用于在清行后将最后一步撤销再送给对方）
int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
int tempGrid[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
int Oval[MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
int tempinfo[105][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
int tempval[105][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };

// 代表分别向对方转移的行
int trans[2][6][MAPWIDTH + 2] = { 0 };

// 转移行数
int transCount[2] = { 0 };

// 运行eliminate后的当前高度
int maxHeight[2] = { 0 };

// 总消去行数的分数之和
int elimTotal[2] = { 0 };

// 连续几回合发生过消去了
int elimCombo[2] = { 0 };

// 一次性消去行数对应分数
const int elimBonus[] = { 0, 1, 3, 5, 7 };

// 给对应玩家的各类块的数目总计
int typeCountForColor[2][7] = { 0 };

const int blockShape[7][4][8] = {
	{ { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
	{ { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
	{ { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
	{ { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
	{ { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
	{ { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
	{ { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
}; // 7种形状(长L| 短L| 反z| 正z| T| 直一| 田格)，4种朝向(上左下右)，8:每相邻的两个分别为x，y

const int rotateBlank[7][4][10] = {
	{ { 1,1,0,0 },{ -1,1,0,0 },{ -1,-1,0,0 },{ 1,-1,0,0 } },
	{ { -1,-1,0,0 },{ 1,-1,0,0 },{ 1,1,0,0 },{ -1,1,0,0 } },
	{ { 1,1,0,0 },{ -1,1,0,0 },{ -1,-1,0,0 },{ 1,-1,0,0 } },
	{ { -1,-1,0,0 },{ 1,-1,0,0 },{ 1,1,0,0 },{ -1,1,0,0 } },
	{ { -1,-1,-1,1,1,1,0,0 },{ -1,-1,-1,1,1,-1,0,0 },{ -1,-1,1,1,1,-1,0,0 },{ -1,1,1,1,1,-1,0,0 } },
	{ { 1,-1,-1,1,-2,1,-1,2,-2,2 } ,{ 1,1,-1,-1,-2,-1,-1,-2,-2,-2 } ,{ -1,1,1,-1,2,-1,1,-2,2,-2 } ,{ -1,-1,1,1,2,1,1,2,2,2 } },
	{ { 0,0 },{ 0,0 } ,{ 0,0 } ,{ 0,0 } }
}; // 旋转的时候需要为空的块相对于旋转中心的坐标

double mark[5][11] = {
	{ 10,  -0.66,  -1.2,   -1.5, -4.3, -5.5, -5.3,  -11,   0.95,   1.3,  0.67 },
	{ 10,  -0.82,  -2.5,   -1.4, -1.2, -2.5, -3.8,  -12,   1,      2.3,  1.2  },
	{ 10,  -1.1,   -2.5,   -2.3, -1.1, -2.2, -2.7,  -7.4,  1,      2.8,  0.6 },
	{ 10,  -1.6,   -3.6,   -2.1, -1.2, -2,   -2.8,  -6.4,  1.1,    4.6,  0.6  },
	//行差，行变换，最高高度，井，  0级洞  1级洞 2级洞  长井 实格评分 消行  迭代系数
};


void retreated(int color)
{
	if (old == 0) return ;
	for (int y = MAPHEIGHT; y >= 1; y--)
		for (int x = 0; x <= MAPWIDTH + 1;x++)
		{
			gridInfo[color][y][x] = tempinfo[old][y][x];
			Oval[y][x] = tempval[old][y][x];
		}
	old--;
}


class Tetris
{
public:
	const int blockType;   // 标记方块类型的序号 0~6
	int blockX;            // 旋转中心的x轴坐标
	int blockY;            // 旋转中心的y轴坐标
	int orientation;       // 标记方块的朝向 0~3
	const int(*shape)[8]; // 当前类型方块的形状定义

	int color;

	Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color)
	{ }

	inline Tetris &set(int x = -1, int y = -1, int o = -1)
	{
		blockX = x == -1 ? blockX : x;
		blockY = y == -1 ? blockY : y;
		orientation = o == -1 ? orientation : o;
		return *this;
	}

	// 判断当前位置是否合法
	inline bool isValid(int x = -1, int y = -1, int o = -1)
	{
		x = x == -1 ? blockX : x;
		y = y == -1 ? blockY : y;
		o = o == -1 ? orientation : o;
		if (o < 0 || o > 3)
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = x + shape[o][2 * i];
			tmpY = y + shape[o][2 * i + 1];
			if (tmpX < 1 || tmpX > MAPWIDTH ||
				tmpY < 1 || tmpY > MAPHEIGHT ||
				gridInfo[color][tmpY][tmpX] != 0)
				return false;
		}
		return true;
	}

	// 判断是否落地
	inline bool onGround()
	{
		if (isValid() && !isValid(-1, blockY - 1))
			return true;
		return false;
	}

	// 将方块放置在场地上
	inline bool place()
	{
		MINHEIGHT = 100;
		if (!onGround()) return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = blockX + shape[orientation][2 * i];
			tmpY = blockY + shape[orientation][2 * i + 1];
			MINHEIGHT = min(MINHEIGHT , tmpY);  
			gridInfo[color][tmpY][tmpX] = 2;
		}

		return true;
	}

	//*************************************begin
	inline void tempplace()
	{
		MINHEIGHT = 100;
		old++;
		for (int y = MAPHEIGHT; y>=1 ; y--)
			for (int x = 0;x <= MAPWIDTH + 1 ; x++)
			{
				tempinfo[old][y][x] = gridInfo[color][y][x];
				tempval[old][y][x] = Oval[y][x];
			}
		
		if (!onGround()) return ;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = blockX + shape[orientation][2 * i];
			tmpY = blockY + shape[orientation][2 * i + 1];
			MINHEIGHT = min(MINHEIGHT , tmpY);  
			gridInfo[color][tmpY][tmpX] = 2;
		}
	}
	//******************************************end

	// 检查能否逆时针旋转自己到o
	inline bool rotation(int o)
	{
		if (o < 0 || o > 3)
			return false;

		if (orientation == o)
			return true;

		int fromO = orientation;
		int i, blankX, blankY;
		while (true)
		{
			if (!isValid(-1, -1, fromO))
				return false;

			if (fromO == o)
				break;

			// 检查旋转碰撞
			for (i = 0; i < 5; i++) {
				blankX = blockX + rotateBlank[blockType][fromO][2 * i];
				blankY = blockY + rotateBlank[blockType][fromO][2 * i + 1];
				if (blankX == blockX && blankY == blockY)
					break;
				if (gridInfo[color][blankY][blankX] != 0)
					return false;
			}

			fromO = (fromO + 1) % 4;
		}
		return true;
	}
};

// 围一圈护城河
void init()
{
	int i;
	for (i = 0; i < MAPHEIGHT + 2; i++)
	{
		gridInfo[1][i][0] = gridInfo[1][i][MAPWIDTH + 1] = -2;
		gridInfo[0][i][0] = gridInfo[0][i][MAPWIDTH + 1] = -2;
	}
	for (i = 0; i < MAPWIDTH + 2; i++)
	{
		gridInfo[1][0][i] = gridInfo[1][MAPHEIGHT + 1][i] = -2;
		gridInfo[0][0][i] = gridInfo[0][MAPHEIGHT + 1][i] = -2;
	}
}

namespace Util
{

	// 检查能否从场地顶端直接落到当前位置
	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o)
	{
		auto &def = blockShape[blockType][o];
		for (; y <= MAPHEIGHT; y++)
			for (int i = 0; i < 4; i++)
			{
				int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
				if (_y > MAPHEIGHT)
					continue;
				if (_y < 1 || _x < 1 || _x > MAPWIDTH || gridInfo[color][_y][_x])
					return false;
			}
		return true;
	}

	// 消去行
	inline void eliminate(int color)
	{
		int &count = transCount[color] = 0;
		int i, j, emptyFlag, fullFlag, firstFull = 1, hasBonus = 0;
		maxHeight[color] = MAPHEIGHT;
		for (i = 1; i <= MAPHEIGHT; i++)
		{
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++)
			{
				if (gridInfo[color][i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag)
			{
				if (firstFull && ++elimCombo[color] >= 3)
				{
					// 奖励行
					for (j = 1; j <= MAPWIDTH; j++)
						trans[color][count][j] = gridInfo[color][i][j] == 1 ? 1 : 0;
					count++;
					hasBonus = 1;
				}
				firstFull = 0;
				for (j = 1; j <= MAPWIDTH; j++)
				{
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					trans[color][count][j] = gridInfo[color][i][j] == 1 ? 1 : 0;
					gridInfo[color][i][j] = 0;
				}
				count++;
			}
			else if (emptyFlag)
			{
				maxHeight[color] = i - 1;
				break;
			}
			else
				for (j = 1; j <= MAPWIDTH; j++)
				{
					gridInfo[color][i - count + hasBonus][j] =
						gridInfo[color][i][j] > 0 ? 1 : gridInfo[color][i][j];
					if (count)
						gridInfo[color][i][j] = 0;
				}
		}
		if (count == 0)
			elimCombo[color] = 0;
		maxHeight[color] -= count - hasBonus;
		elimTotal[color] += elimBonus[count - hasBonus];
	}

	// 转移双方消去的行，返回-1表示继续，否则返回输者
	inline int transfer()
	{
		int color1 = 0, color2 = 1;
		if (transCount[color1] == 0 && transCount[color2] == 0)
			return -1;
		if (transCount[color1] == 0 || transCount[color2] == 0)
		{
			if (transCount[color1] == 0 && transCount[color2] > 0)
				swap(color1, color2);
			int h2;
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];
			if (h2 > MAPHEIGHT)
				return color2;
			int i, j;

			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];

			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];
			return -1;
		}
		else
		{
			int h1, h2;
			maxHeight[color1] = h1 = maxHeight[color1] + transCount[color2];//从color1处移动count1去color2
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];

			if (h1 > MAPHEIGHT) return color1;
			if (h2 > MAPHEIGHT) return color2;

			int i, j;
			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];

			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];

			for (i = h1; i > transCount[color2]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = gridInfo[color1][i - transCount[color2]][j];

			for (i = transCount[color2]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = trans[color2][i - 1][j];

			return -1;
		}
	}

	// 颜色方还能否继续游戏
	inline bool canPut(int color, int blockType)
	{
		Tetris t(blockType, color);
		for (int y = MAPHEIGHT; y >= 1; y--)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++)
				{
					t.set(x, y, o);
					if (t.isValid() && checkDirectDropTo(color, blockType, x, y, o))
						return true;
				}
		return false;
	}

	// 打印场地用于调试
	inline void printField()
	{
#ifndef _BOTZONE_ONLINE
		static const char *i2s[] = {
			"~~",
			"~~",
			"  ",
			"[]",
			"##"
		};
    	cout << "~~：墙，[]：块，##：新块" << endl;
		for (int y = MAPHEIGHT + 1; y >= 0; y--)
		{
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo[0][y][x] + 2];
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[gridInfo[1][y][x] + 2];
			cout << endl;
		}
#endif
	}
}
/*
bool iscanputblock(int X, int Y, int o, int blockType, int currBotColor){
	//复制当前地图
	int tempGrid[2][MAPHEIGHT + 5][MAPWIDTH + 5];
	for (int j = 1; j <= MAPHEIGHT; j++) {
		for (int k = 1; k <= MAPWIDTH; k++) {
			tempGrid[currBotColor][j][k] = gridInfo[currBotColor][j][k];
		}
	}


	auto &def = blockShape[blockType][o];
	int num = 0;
	for (int i = 0; i <= MAPWIDTH + 1; i++) {
		tempGrid[currBotColor][0][i] = 3;
	}
	
	//判断是否为非法落地
	for (int i = 0; i < 4; i++) {
		if(tempGrid[currBotColor][Y - 1 + def[i * 2 + 1]][X + def[i * 2]] != 0) num++;
	}
	if(num == 0) return false;
	return true;
}*/


void add(int color,int x,int val)
{
	typeCountForColor[color][x] +=val;
	maxCount[color] = *max_element(typeCountForColor[color],typeCountForColor[color] + 7);
	minCount[color] = *min_element(typeCountForColor[color],typeCountForColor[color] + 7);
}

int diry[5]={1,-1,0,0};
int dirx[5]={0,0,1,-1};

void findallfx(int blockType,int color)
{
	Tetris hzblock(blockType,color);
	memset(Oval,0,sizeof(Oval));
	queue<Tetris> q;
	//遍历棋盘，可垂直下落到达的任意位置入队
	for (int y = 1;y <= MAPHEIGHT; y++)
			for (int x = 1; x<= MAPWIDTH; x++)
				for (int o = 0; o < 4 ; o++)
				{
					if (hzblock.set(x, y, o).isValid()
						&& Util::checkDirectDropTo(color, blockType, x, y, o) 
						)
						{
							Oval[y][x] |= (1<<o);
							q.push(Tetris(blockType, color).set(x, y, o));
						}
				}
	while (!q.empty())
	{
		Tetris nowpoint = q.front();q.pop();
		//对每个可行位置进行旋转
		if (nowpoint.rotation( (nowpoint.orientation + 1)% 4)
			&& (!(Oval[ nowpoint.blockY ][ nowpoint.blockX ] & (1 << ( (nowpoint.orientation + 1)% 4 ) )) )
			)
			{	
				Tetris gopoint = Tetris(blockType ,color ).set(nowpoint.blockY,nowpoint.blockX, (nowpoint.orientation + 1)% 4);
				if (gopoint.isValid())
				{
					q.push(gopoint);
					Oval[gopoint.blockY][gopoint.blockX] |= 1 << gopoint.orientation;
				} 
			}
		//对每个可行位置进行移动
		for (int i = 0 ; i < 4; i++)
		{
			int _x = nowpoint.blockX +dirx[i];
			int _y = nowpoint.blockY +diry[i];
			Tetris gopoint = Tetris(blockType,color).set(_x,_y,nowpoint.orientation);
			if (gopoint.isValid() && !(Oval[_y][_x] & ( 1 << gopoint.orientation) ))
			{
				q.push(gopoint);
				Oval[_y][_x] |= (1 << gopoint.orientation);
			}
		}
	}
	//剪枝
	for (int y = 1;y <= MAPHEIGHT; y++)
		for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0;o < 4; o++)
				if (Oval[y][x] & (1 << o) )
				{
					if (blockType == 6 && o != 0) Oval[y][x] -= (1<<o);  //方块的旋转数量剪枝到1
					if ((blockType == 2 || blockType == 3 || blockType == 5 )&& o <= 1) Oval[y][x] -= (1<<o); // SZ长条旋转数量剪枝到2
				}
}


double soildmark[15] = { 0, 0.6, 0.4, 0.2, 0.1, 0, 0, 0.1, 0.2, 0.4, 0.6, 0 };
//每列实格数量的系数   (两边高中间低)
int soildpoint[MAPWIDTH + 5];
int well[MAPWIDTH + 5];
int dep[MAPWIDTH + 5];
int lastpoint[10];
int TempGrid[MAPHEIGHT + 5][MAPWIDTH + 5];
//局面评估
double calculate(int currBotColor)
{
	memset(soildpoint,0,sizeof(soildpoint));//记录第x行从上往下当前实格数
	memset(well,-1,sizeof(well));
	memset(dep,0,sizeof(dep));

	double MAXHEIGHT = 0;
	//复制当前地图
	for (int y = MAPHEIGHT; y >= 0; y--) 
		for (int x = 0; x <= MAPWIDTH + 1; x++) 
		{
			TempGrid[y][x] = gridInfo[currBotColor][y][x];
			if (TempGrid[y][x] != 0)
			{
				TempGrid[y][x] = 1;
				if (!MAXHEIGHT && x != 0 && x != MAPWIDTH + 1)
					MAXHEIGHT = y; //计算最高高度
			}
		}
		

	double boardRowTransitions = 0;//行变换
	double Heightdifferent = 0;
	double Holes[4]={ 0 };//洞分类   被埋在1格以下，2格以下，2格以上/0级，1级，2级


	for (int y = MAPHEIGHT; y >= 0 ; y--)
	{
		bool flag = 0;
		//计算行变换
		for (int x = 0; x <= MAPWIDTH;x++)
			if ( TempGrid[y][x] != TempGrid[y][x+1]) 
				boardRowTransitions++;

		//对当前行所有空洞进行分类
		for (int x = 1; x <= MAPWIDTH;x++)
		{
			if (TempGrid[y][x] == 1) soildpoint[x]++;
			else
			if ( soildpoint[x] != 0)
			{
				flag = 1;
				TempGrid[y][x] = 2; 
				if (soildpoint[x] <= 3) Holes[soildpoint[x]-1]++;
				else Holes[2]++;
			}
		}

		//评估表面空格的形状为 ..1/1../
		well[1] = 1;
		well[MAPWIDTH] = 0;
		for (int x = 1;x <= MAPWIDTH; x++)
			if (TempGrid[y][x] == 0 && !soildpoint[x] && well[x] == -1)
			{
				if (TempGrid[y][x - 1] == 0 && TempGrid[y][x + 1] && x != MAPWIDTH)
					well[x] = 0;
				if (TempGrid[y][x + 1] == 0 && TempGrid[y][x - 1] && x != 1)
					well[x] = 1;
			}
		//计算表面井的深度	
		for (int x = 1;x <= MAPWIDTH; x++)
			if (TempGrid[y][x] == 0 && !soildpoint[x])
				if(TempGrid[y][x] == 0 && !soildpoint[x] && TempGrid[y][x - 1] && TempGrid[y][x + 1])
					dep[x]++;

		//当前高度存在空列 增加其他行评分
		if (!flag && y)
		{
			Heightdifferent++;
			for (int x = 1; x <= MAPWIDTH; x++)
				if (TempGrid[y][x] == 1)
					soildpoint[x]++;
		}
	}


	memset(lastpoint,0,sizeof(lastpoint));

	//接下去块的概率
	for (int i = 0; i < 7 ; i++)
		for (int j = 0 ; j < 7 ; j++)
			if (i!=j)
				lastpoint[i] += typeCountForColor[currBotColor][i] + 2 - typeCountForColor[currBotColor][j];
	//得分越高概率越小


	//评估每列表面的井  
	double boardWells = 0;
	for (int x = 1; x <= MAPWIDTH ; x++)
		if ( dep[x] )
		{
			double tmp = (1 + dep[x])*(dep[x]) / 2;//1加到深度的和
			//特殊判断
			if (turnID <= 30)
			{
				if ( dep[x] == 2)
				{
					if (well[x] == 0 && lastpoint[0] > 10) boardWells += tmp/2;         //表面深度为2且状态为..1 小概率出左L
					if (well[x] == 1 && lastpoint[1] > 10) boardWells += tmp/2;         //表面深度为2且状态为1.. 小概率出右L
					if (well[x] == 0 && lastpoint[0] <= 4) dep[x]= 0,lastpoint[0] += 6; //表面深度为2且状态为..1 大概率左L   取消深度 降低左L概率
					if (well[x] == 1 && lastpoint[1] <= 4) dep[x]= 0,lastpoint[1] += 6; //表面深度为2且状态为1.. 大概率右L   取消深度 降低右L概率
					if (well[x] ==-1 && min(lastpoint[0],lastpoint[1]) <= 6) dep[x] = 0;//表面深度为2且两边等高  L概率高     取消深度
				}
				else
				if (dep[x] >= 3)
				{
					if (lastpoint[5] > 10) boardWells += tmp;                //表面深度>=3  小概率长条
					if (lastpoint[5] <= 4) dep[x] = 0,lastpoint[5] += 6;     //表面深度>=3  大概率长条   取消深度 降低L概率
				}
			}
			boardWells += (1 + dep[x])*(dep[x]) / 2;
		}

	int longwell = -1;
	for (int x = 1; x <= MAPWIDTH; x++)
		if (dep[x] >= 3)
			longwell++;
	if (longwell < 0) longwell = 0; //长井有一个不要紧
	

	//每列实格数量评分，越像山谷评分越高
	double soildpointscore = 0;
	for (int x = 1; x <= MAPWIDTH; x++)
		soildpointscore += soildpoint[x] * soildmark[x] / 2;

	MAXHEIGHT += 0.8 * MINHEIGHT;

	
	double allans[]={Heightdifferent,boardRowTransitions,MAXHEIGHT,boardWells,
					 Holes[0],Holes[1],Holes[2],(double)longwell,soildpointscore};
	
	double allscore = 0;
	for (int i = 0 ; i < 9 ; i++) 
		allscore += mark[ Strategy ][ i ] * allans[i];
	return allscore;
}



struct XX {
	int	x,y,o;
	double value;
}A[100];

bool operator<( XX a, XX b ) {return(a.value>b.value);}

int allafter, temporary[10];
int afterpoint[600][10];

int DFN[] = { 6, 5, 3, 2, 4, 1, 0 };
//dfs搜索所有接下去deepth层可能出现的所有情况存放在aftherpoint
void findafterpoint(int color,int deep,int deepth)
{
	if (deep == deepth)
	{
		allafter++;
		for (int i = 0;i < deepth;i++) afterpoint[allafter][i] = temporary[i];
		return ;
	}
	for (int i = 0; i < 7 ;i++)
		if ((maxCount[color] - minCount[color] < 2 || typeCountForColor[color][DFN[i]] != maxCount[color]))
		{
			add(color, DFN[i], 1);
			temporary[deep] = DFN[i];
			findafterpoint(color,deep+1,deepth);
			add(color, DFN[i], -1);
		}
}


double solve(int blockType,int color,int deep,int deepth,int afternow,double sum)
{
	Tetris hzblock(blockType,color);
	findallfx(blockType,color);
	double ans = -200000;
	XX A[100];
	int lengthA = 0;
	for (int y = 1; y <= MAPHEIGHT ; y++)
		for (int x = 1; x <=MAPWIDTH ; x++)
			for (int o = 0; o < 4; o++)
				if(hzblock.set(x, y, o).onGround() && (Oval[y][x] & (1 << o )))
				{
					add(color,blockType,1);
					hzblock.set(x,y,o).tempplace();
					Util::eliminate(color);
					double score = mark[ Strategy ][ 9 ] * elimCombo[color];
					if (BESTHIGHT[color] >= 18 || BESTHIGHT [ 1 - color ] >= 18 ) score *= 5;
					double tmp = score;
					if (deep < deepth) tmp += calculate(color) / mark[ Strategy ][ 10 ] * 2;
					else tmp += calculate(color);
					A[lengthA++] = ((XX){x, y, o, tmp});
					if (deep == deepth && tmp > ans)
						ans = tmp;
					retreated(color);
					add(color, blockType, -1);
					if (ans > sum) return ans;
				}
	if (lengthA == 0) return deep * 1000 - 100000;
	if (deep < deepth)
	{
		sort(A,A+lengthA);
		for (int i = 0,j = 1; i < lengthA; i++ , j++)
		{
			add(color, blockType, 1);
			hzblock.set(A[i].x, A[i].y, A[i].o).tempplace();
			Util::eliminate(color);
			double tmp = A[i].value + solve(afterpoint[afternow][deep],color,deep + 1,deepth,afternow,sum - A[i].value);
			if (tmp > ans)
				ans = tmp;
			retreated(color);
			add(color,blockType,-1);
			if (ans > sum) return ans;
			if (j == FINDWIDTH) break;
		}
	} 
	return ans;
}


void LASTAIGO(int blockType,int color)
{
	double ans = -2147483647;
	
	allafter = 0;
	int deepth = 1;
	findafterpoint(color , 0 , deepth);
	//搜索所有接下去deepth层所有方块出现情况
	
	Tetris hzblock(blockType, color);
	findallfx(blockType, color);
	XX A[100];
	int lengthA = 0;
	for (int y = 1;y <= MAPHEIGHT ; y++)
		for (int x = 1; x <=MAPWIDTH ; x++)
			for (int o = 0; o < 4; o++)
			{
				if (hzblock.set(x, y, o).onGround() && (Oval[y][x] & (1 << o) )  )
				{
					hzblock.set(x, y, o).tempplace();
					Util::eliminate(color);
					double nowscore = mark[Strategy][ 9 ] * elimCombo[color] ;
					if (BESTHIGHT[color] >= 18 || BESTHIGHT [ 1 - color ] >= 18 ) nowscore *= 5;
					//一方玩家快满了，提高消行得分
					nowscore += calculate(color) / mark [Strategy][ 10 ] * 2; //后期增加系数
					A[lengthA++] = ((XX){x, y, o, nowscore});
					retreated(color); 
				}
			} 
	sort(A,A+lengthA);
	for (int i = 0,j = 1;i < lengthA ; i++,j++)
	{
		double sum = 100000 ;
		hzblock.set(A[i].x,A[i].y,A[i].o).tempplace();
	//	if (A[i].x == 7 && A[i].y == 18) cout<<"a"<<A[i].value<<" "<<A[i].o<<endl;
	//	if (A[i].x == 5 && A[i].y == 19) cout<<"b"<<A[i].value<<" "<<A[i].o<<endl;
		Util::eliminate(color);
		for (int ii = 1; ii <= allafter; ii++)
		{
			double tmp = A[i].value + solve(afterpoint[ii][0],color,1,deepth,ii,100000 - A[i].value);
			sum = min(sum , tmp);
			if (sum <= ans) break;
		}
	//	if (A[i].x == 7 && A[i].y == 18) cout<<sum<<endl;
	//	if (A[i].x == 5 && A[i].y == 19) cout<<sum<<endl;
		retreated(color);
		if (sum > ans)
		{
			ans = sum;
			finalX = A[i].x;
			finalY = A[i].y;
			finalO = A[i].o;
		}
		if (j == 2 * FINDWIDTH) break; //搜索宽度
	}
}


void AIGO(int blockType,int color)
{
	double ans = -2147483647;
	
	allafter = 0;
	int deepth = 1;
	findafterpoint(color , 0 , deepth);
	//搜索所有接下去deepth层所有方块出现情况
	
	Tetris hzblock(blockType, color);
	findallfx(blockType, color);
	XX A[100];
	int lengthA = 0;
	for (int y = 1;y <= MAPHEIGHT ; y++)
		for (int x = 1; x <=MAPWIDTH ; x++)
			for (int o = 0; o < 4; o++)
			{
				if (hzblock.set(x, y, o).onGround() && (Oval[y][x] & (1 << o) )  )
				{
					hzblock.set(x, y, o).tempplace();
					Util::eliminate(color);
					double nowscore = mark[Strategy][ 9 ] * elimCombo[color] ;
					if (BESTHIGHT[color] >= 18 || BESTHIGHT [ 1 - color ] >= 18 ) nowscore *= 5;
					//一方玩家快满了，提高消行得分
					nowscore += calculate(color) / mark [Strategy][ 10 ];
					A[lengthA++] = ((XX){x, y, o, nowscore});
					retreated(color); 
				}
			}

	sort(A,A+lengthA);
	for (int i = 0,j = 1;i < lengthA ; i++,j++)
	{
		double sum = 100000,tot = 0;
		hzblock.set(A[i].x,A[i].y,A[i].o).tempplace();
		Util::eliminate(color);
		for (int ii = 1; ii <= allafter; ii++)
		{
			double tmp = A[i].value + solve(afterpoint[ii][0],color,1,deepth,ii,100000);
			tot += tmp;
			sum = min(sum , tmp);
		}
		sum = sum + tot / allafter ; 
		retreated(color);
		if (sum > ans)
		{
			ans = sum;
			finalX = A[i].x;
			finalY = A[i].y;
			finalO = A[i].o;
		}
		if (j == 2 * FINDWIDTH) break; //搜索宽度
	}
}

void attack(int color)
{
	int lastblocktype = nextTypeForColor[color];
	allafter = 0;
	int deepth = 2;
	findafterpoint(color,0,deepth);
	double ans = 100000;
	for (int i = allafter ; i > 0 ; i--)
	{
		add(color,lastblocktype, 1);
		double sum = solve(lastblocktype,color,0,deepth + 1 , i ,ans);
		add(color,lastblocktype,-1);
		if (sum < ans)
		{
			ans = sum;
			blockForEnemy = afterpoint[i][0];
		}
	}
}


int main()
{
	// 加速输入
	istream::sync_with_stdio(false);
	R = (double)rand() / RAND_MAX + 0.5;
	srand(time(NULL));
	init();

	cin >> turnID;
	if (turnID <= 15) Strategy = 0;else
	if (turnID <= 30) Strategy = 1;else
	if (turnID <= 45) Strategy = 2;else Strategy = 3;
	// 先读入第一回合，得到自己的颜色
	// 双方的第一块肯定是一样的
	cin >> blockType >> currBotColor;
	enemyColor = 1 - currBotColor;
	nextTypeForColor[0] = blockType;
	nextTypeForColor[1] = blockType;
	add(0,blockType,1);
	add(1,blockType,1);

	// 然后分析以前每回合的输入输出，并恢复状态
	// 循环中，color 表示当前这一行是 color 的行为
	// 平台保证所有输入都是合法输入
	for (int i = 1; i < turnID; i++)
	{
		int currTypeForColor[2] = { nextTypeForColor[0], nextTypeForColor[1] };
		int x, y, o;

		//读取己方数据
		cin >> blockType >> x >> y >> o;
		Tetris myBlock(currTypeForColor[currBotColor], currBotColor);
		myBlock.set(x, y, o).place();
		add(enemyColor, blockType, 1);
		nextTypeForColor[enemyColor] = blockType;

		// 然后读自己的输入，也就是对方的行为
		// 裁判给自己的输入是对方的最后一步
		cin >> blockType >> x >> y >> o;
		
		Tetris enemyBlock(currTypeForColor[enemyColor], enemyColor);
		enemyBlock.set(x, y, o).place();
		nextTypeForColor[currBotColor] = blockType;
		add(currBotColor, blockType, 1);
		
		// 检查消去
		Util::eliminate(0);
		Util::eliminate(1);
		// 进行转移
		Util::transfer();
	}
	// 做出决策（你只需修改以下部分）

	//计算每列最高高度
	for (int i = 0 ; i < 2 ; i++)
		for (int y = MAPHEIGHT; y >= 1 && !BESTHIGHT[i];y--)
			for (int x = 1; x <= MAPWIDTH;x++)
				if (gridInfo[i][y][x])
					BESTHIGHT[i]=y;
	/*findallfx(blockType, currBotColor);
	for (int i = MAPHEIGHT; i >= 1; i--) {
		for (int j = 1; j <= MAPWIDTH; j++) {
			cout<<Oval[i][j]<<" ";
		}
		cout<<endl;
	}*/
	if (Strategy == 0) AIGO(blockType,currBotColor);
	else LASTAIGO(blockType,currBotColor);
	Tetris hzblock(blockType,currBotColor);
	hzblock.set(finalX,finalY,finalO).tempplace();
	Util::printField();
	Util::eliminate( currBotColor );
	Util::transfer();

	
	attack(enemyColor);

		// 决策结束，输出结果（你只需修改以上部分）
	cout << blockForEnemy << " " << finalX << " " << finalY << " " << finalO;
	return 0;
}