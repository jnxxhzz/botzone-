/**
* Tetris2 样例程序
* 20171031更新：将elimBonus[count]改为elimBonus[count - hasBonus]。
* 20171027更新：将trans数组的第二维长度从4加大到6，感谢kczno1用户的指正。
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
#include "jsoncpp/json.h" 
using namespace std;

#define MAPWIDTH 10
#define MAPHEIGHT 20

// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
int currBotColor;
int enemyColor;

// 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界
// （2用于在清行后将最后一步撤销再送给对方）
int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
int tempGrid[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
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
	inline bool place(int res)
	{
		//begin
		PH = 100;
		if (res)
		{
			RESID++;
			for (int y = MAPHEIGHT;i>=1; y--)
				for (int x = 0;x <= MAPWIDTH + 1 ; x++)
				{
					tempres[tempid][y][x] = gridInfo[color][y][x];
					tempress[tempid][y][x] = Oval[y][x];
				}
		}
		//end;
		if (!onGround())
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = blockX + shape[orientation][2 * i];
			tmpY = blockY + shape[orientation][2 * i + 1];
			PH = min(PH , tempY);   //PH
			gridInfo[color][tmpY][tmpX] = 2;
		}
		return true;
	}

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
	void eliminate(int color)
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
	int transfer()
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
//		cout << "~~：墙，[]：块，##：新块" << endl;
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

void add(int color,int x,int val)
{
	CCnt[color][x] +=val;
	maxCount[color] = *max_element(CCnt[color],CCnt[color] + 7);
	minCount[color] = *min_element(CCnt[color],CCnt[color] + 7);
}


void bfs(int blockType,int color)
{
	Tetris hzblock(blockType,color);
	memset(Oval,0,sizeof(Oval));
	queue<Tetris> q;
	for (int y = 1;y <= MAPHEIGHT; y++)
			for (int x = 1; x<= MAPWIDTH; y++)
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
		if (nowpoint.rotation( (nowpoint.orientation + 1)% 4)
			&& (!(Oval[ nowpoint.blockY ][ nowpoint.blockX ] & (1 << ( (nowpoint.orientation + 1)% 4 ))))
			)
			{	
				Tetris gopoint = Tetris(blockType ,color ).set(nowpoint.blockY,nowpoint.blockX, (nowpoint.orientation + 1)% 4)
				if (gopint.isValid()) 
				{
					q.push(gopoint);
					Oval[gopoint.blockY][gopoint.blockX] |= 1 << gopoint.orientation;
				}
			}
		for (int i = 0 ; i < 4; i++)
		{
			int _x = nowpoint.blockX +dirx[i];
			int _y = nowpoint.blockY +diry[i];
			Tetris gopoint = Tetris(blockType,color).set(_x,_y,nowpoint.orientation);
			if (gopoint.isValid() && (!Oval[y][x] & ( 1 << gopoint.orientation) ))
			{
				q.push(gopoint);
				Oval[y][x] |= 1<<gopoint.orientation;
			}
		}
	}
	for (int y = 1;y <= MAPHEIGHT; y++)
		for (int x = 1; x<=MAPWIDTH; x++)
			for (int o = 0;o < 4; o++)
				if (Oval[y][x] & (1<<o) )
				{
					if (blockType == 6 && o != 0) Oval[y][x] -= (1<<o);
					if ((blockType == 2 || blockType == 3 || blockType == 5 )&& o <= 1)
						Oval[y][x] -= (1<<o);
				}
}


int top, st[505][10], tmp[10];
struct XX {
	int	x,y,o;
	double value;
};

bool operator<( XX a, XX b ) {return(a.tmp>b.tmp);}


int DFN[] = { 6, 5, 3, 2, 4, 1, 0 };
void dfs(int color,int deep,int deepth)
{
	if (deep == deepth)
	{
		top++;
		for (int i = 0;i < deepth;i++) st[top][i] = tmp[i];
	}
	for (int i = 0; i < 7 ;i++)
		if ((maxCount[color] - minCount[color] < 2 || CCnt[color][DFN[i]] != maxCount[color]))
		{
			// i -> dfn[i]
			add(color, i, 1);
			add(color, i, 1);
			tmp[deep] = i;
			dfs(color,deep+1,deepth);
			add(color, i, -1);
		}
}


void AIGO(int blockType,int color)
{
	double MX = -2147483647;
	top = 0;
	int deepth = 2;
	dfs(color , 0 , deepth);
	dfsW = 4;
	Tetris hzblock(blockType, color);
	bfs(blockType, color);
	data A[100];
	int lengthA = 0;
	for (int y = 1;y <= MAPHEIGHT ; y++)
		for (int x = 1; x <=MAPWIDTH ; x++)
			for (int o = 0; o < 4; o++)
			{
				if (hzblock.set(x, y, o).onGround && (Oval[y][x] & (1 << o) )  )
				{
					hzblock.set(x, y, o).place(1);
					Util::eliminate(color);
					double nowscore = C[0][N - 2] * elimCombo[color] ;
					if (BESTHIGHT[color] >= 18 || BESTHIGHT [ 1 - color] >= 18 ) score *= 5;
					score += calculate(color) / C [0][ N - 1 ] * 2;
					A[lengthA++] = ((data){x, y, o, score});
					retreated(color); //*****
				}
			}
	sort(A,A+lengthA);
	for (int i = 0,j = 0;i < lengthA ; i++ , j++)
	{
		double sum=1000000;
		hzblock.set(A[i].x,A[i].y,A[i].o).place( 1 );
		Util::eliminate(color);
		for (int ii = 1; ii <= top; ii++)
		{
			double tmp = A[i].value + solve(st[t][0],color,0,D,t,sum-A[i].value);
			sum = min(sum , tmp);
			if (sum < ans) break;
		}
		retreated(color);
		if (summin > ans)
		{
			ans = sum;
			finalX = A[i].x;
			finalY = A[i].y;
			finalO = A[i].o;
		}
		if (j == 5) break;
	}
}












bool iscanputblock(int X, int Y, int o, int blockType, int currBotColor){
	//复制当前地图
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
}


int cal(int X, int Y, int o, int blockType, int currBotColor) {
	//int grd[] = {0, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 7, 7 };
	//int grd2[] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10};

	
	//复制当前地图
	for (int j = 1; j <= MAPHEIGHT; j++) {
		for (int k = 1; k <= MAPWIDTH; k++) {
			tempGrid[currBotColor][j][k] = gridInfo[currBotColor][j][k];
		}
	}
	//围一圈边界
	for (int i = 0; i <= MAPWIDTH + 1; i++) {
		tempGrid[currBotColor][0][i] = tempGrid[currBotColor][MAPHEIGHT + 1][i] = 3;
	}
	for (int i = 0; i <= MAPHEIGHT + 1; i++) {
		tempGrid[currBotColor][i][0] = tempGrid[currBotColor][i][MAPWIDTH +1] = 3;
	}

	auto &def = blockShape[blockType][o];

	
	//模拟放入当前块到指定位置
	for (int i = 0; i < 4; i++) {
		tempGrid[currBotColor][Y + def[i * 2 + 1]][X + def[i * 2]] = 4;
	}

	//模拟消行
	int ans = 0;
	int nowboks = 0;
	for (int j = MAPHEIGHT; j >= 1; j--) {
		int cnt =  0;
		int cntblocks = 0;
		for (int i = 1; i <= MAPWIDTH; i++) {
			if(tempGrid[currBotColor][j][i] != 0) cnt++;
			if(tempGrid[currBotColor][j][i] == 4) cntblocks++;
		}
		if(cnt == 10) {
			ans++;
			nowboks += cntblocks;
			for (int k = j; k < MAPHEIGHT; k++) {
				for (int i = 1; i <= MAPWIDTH; i++) {
					tempGrid[currBotColor][k][i] = tempGrid[currBotColor][k + 1][i];
				}
			}
			for (int i = 1; i <= MAPWIDTH; i++) {
				tempGrid[currBotColor][MAPHEIGHT][i] = 0;
			}
		}
	}

	int erodedPieceCellsMetric = ans * nowboks ;   //特征值

	//计算行列变换次数
	
	int boardRowTransitions = 0 ;  //特征值	
	for (int j = 1; j <= MAPHEIGHT; j++) {
		//if (tempGrid[ currBotColor ][ j ][ MAPWIDTH ] != 1) boardRowTransitions++;
		// tempGrid[ currBotColor ][ j ][ 0 ] = tempGrid[ currBotColor ][ j ][ MAPWIDTH + 1 ] = 1 ;
		for (int k = 1; k <= MAPWIDTH + 1; k++) {
			if (tempGrid[ currBotColor ][ j ][ k ] != tempGrid[ currBotColor ][ j ][ k - 1]) 
				boardRowTransitions++ ;
		}
	}

	int boardColTransitions = 0 ;  //特征值
	for (int k = 1; k <= MAPWIDTH; k++) {
		//if (tempGrid[ currBotColor ][ MAPHEIGHT ][ k ] != 1) boardColTransitions++;
		for (int j = 1; j <= MAPHEIGHT + 1; j++){
			if (tempGrid[ currBotColor ][ j ][ k ] != tempGrid[ currBotColor ][ j - 1 ][ k ]) 
				boardColTransitions++ ;
		}
	}

	//计算空洞
	int boardBuriedHoles = 0;  //特征值
	int Holeslines = 0;
	int besthight = 0;
	for (int i = 1; i <= MAPWIDTH; i++) {
		bool ok = false;
		int isholeline = 0;
		for (int j = MAPHEIGHT; j >= 1; j--) {
			if(tempGrid[currBotColor][j][i] != 0) ok = true;
			if(ok && tempGrid[currBotColor][j][i] == 0) {
				boardBuriedHoles += tempissolid[ currbotcolor ][ j ][ i ];
				isholeline = 1;
				if (j > besthight) besthight = j;
			}
		}
		Holeslines += isholeline;
	}

	/*//计算每行高度
	int cowHight[MAPWIDTH + 5];
	cowHight[0] = cowHight[MAPWIDTH+1] = MAPHEIGHT ; 
	memset(cowHight, 0, sizeof(cowHight));
	for (int i = 1; i <= MAPWIDTH; i++) {
		for (int j = MAPHEIGHT; j >= 1; j--) {
			if (tempGrid[currBotColor][j][i] == 1) {
				cowHight[i] = j;
				break;
			}
		}
	}*/
	
	int boardWells = 0;   //特征值
	//计算井的深度和
	/*for (int j = 0; j <=MAPHEIGHT ; j++)	
		for (int i = 1; i <= MAPWIDTH; i++)
			if (tempGrid [ currBotColor ][ j ][ i ] == 1 && tempGrid [ currBotColor ][ j ][ i + 1 ] == 1 && tempGrid [ currBotColor ][ j ][ i - 1 ] == 1 && 
				tempGrid [ currBotColor ][ j + 1 ][ i ] == 0 && tempGrid [ currBotColor ][ j + 1 ][ i + 1 ] == 1 && tempGrid [ currBotColor ][ j + 1 ][ i - 1 ] == 1 )
			{
				int k;
				int bokleft = 0 ;
				int bokright = 0 ;
				k = j;
				while (tempGrid [ currBotColor ][ k + 1 ][ i - 1] == 1 ) k++,bokleft++;
				k = j;
				while (tempGrid [ currBotColor ][ k + 1 ][ i + 1] == 1 ) k++,bokright++;
				k = min(bokleft,bokright);
				boardWells += ( 1 + k ) * k / 2;
			}*/
	int depth = 0;
	for (int i = 1;i <= MAPWIDTH ; i++){
		depth = 0;
		for (int j = MAPHEIGHT;j >= 0 ;j-- ){
			if (tempGrid [ currBotColor][ j ][ i ] == 0 &&
				tempGrid [ currBotColor][ j ][ i - 1 ] == 1 &&
				tempGrid [ currBotColor][ j ][ i + 1 ] == 1  
				)
				depth++;
			else
				boardWells += (1 + depth)* depth / 2, depth = 0;
		}
		boardWells += (1 + depth)* depth / 2, depth = 0;
	}

	int bestholes = 0;
	for (int i = besthight ; i <=MAPHEIGHT ; i++)
		for (int j = 0 ; j <= MAPWIDTH ; j++)
			if (tempGrid[currbotcolor][i][j] != 0)
				bestholes ++;

	int landingHeight = Y - ans; // 特征值 

	double rating_map =( - boardRowTransitions * 80 
						 - boardColTransitions * 80
						 - boardBuriedHoles    * 60
						 - Holeslines          * 380
						 - boardWells          * 100
						 - bestholes           * 5
						 - 
						 	)


	/*double  rating = ( -4.500158825082766 )  * landingHeight           //下落高度
				   + ( 3.4181268101392694 )  * erodedPieceCellsMetric  //消行个数
			       + ( -3.2178882868487753)  * boardRowTransitions     //行变换
			       + ( -9.348695305445199 )  * boardColTransitions     //列变换
			       + ( -7.899265427351652 )  * boardBuriedHoles        //空洞个数
			       + ( -3.3855972247263626)  * boardWells;             //井*/

	/*double  rating =   (-1)  * landingHeight           //下落高度
				     + ( 1)  * erodedPieceCellsMetric  //消行个数
			         + (-1) * boardRowTransitions      //行变换
			         + (-1)  * boardColTransitions     //列变换
			         + (-4)  * boardBuriedHoles        //空洞个数
			         + (-1) * boardWells;              //井*/

	return rating;
}


//寻找可行路径
struct findway{
	int nowx,nowy;
};

int findwayvis[MAPWIDTH + 5] [MAPHEIGHT + 5];

bool checkcango(int X, int Y, int o,int blockType, int currBotColor){
	auto &def = blockShape[blockType][o];
	for (int i = 0 ; i < 4 ; i++){
		int _x = X + def[i * 2];
		int _y = Y + def[i * 2 + 1];
		if (gridInfo[currBotColor][_y][_x] == 1|| 
			_x < 1 || _x > MAPWIDTH ||
			_y < 1 ){
			return false;
		}
	}
	return true;
}

bool canfindway(int X, int Y, int o, int blockType, int currBotColor){
	queue<findway> q;
	memset(findwayvis,0,sizeof(findwayvis));
	findway nowpoint,gopoint;
	nowpoint.nowx=X;
	nowpoint.nowy=Y;
	q.push(nowpoint);
	findwayvis[nowpoint.nowx][nowpoint.nowy] = 1;
	while (!q.empty()){
		nowpoint = q.front();q.pop();
		if (nowpoint.nowy>=MAPHEIGHT) return true;
		if (findwayvis[nowpoint.nowx][nowpoint.nowy + 1] == 0 && checkcango( nowpoint.nowx, nowpoint.nowy + 1, o , blockType, currBotColor)){
			gopoint.nowx = nowpoint.nowx;
			gopoint.nowy = nowpoint.nowy + 1; 
			q.push(gopoint);
			findwayvis[gopoint.nowx][gopoint.nowy] = 1;
			if (gopoint.nowy>=MAPHEIGHT) return true;
		}
		if (findwayvis[nowpoint.nowx - 1][nowpoint.nowy] == 0 && checkcango( nowpoint.nowx - 1, nowpoint.nowy, o , blockType, currBotColor)){
			gopoint.nowx = nowpoint.nowx - 1;
			gopoint.nowy = nowpoint.nowy; 
			q.push(gopoint);
			findwayvis[gopoint.nowx][gopoint.nowy] = 1;
			if (gopoint.nowy>=MAPHEIGHT) return true;
		}
		if (findwayvis[nowpoint.nowx + 1][nowpoint.nowy] == 0 && checkcango( nowpoint.nowx + 1, nowpoint.nowy, o , blockType, currBotColor)){
			gopoint.nowx = nowpoint.nowx + 1;
			gopoint.nowy = nowpoint.nowy; 
			q.push(gopoint);
			findwayvis[gopoint.nowx][gopoint.nowy] = 1;
			if (gopoint.nowy>=MAPHEIGHT) return true;
		}
	}
	return false;
}




int main()
{
	// 加速输入
	istream::sync_with_stdio(false);
	srand(time(NULL));
	init();

	int turnID, blockType;
	int nextTypeForColor[2];
	cin >> turnID;

	// 先读入第一回合，得到自己的颜色
	// 双方的第一块肯定是一样的
	cin >> blockType >> currBotColor;
	enemyColor = 1 - currBotColor;
	nextTypeForColor[0] = blockType;
	nextTypeForColor[1] = blockType;
	typeCountForColor[0][blockType]++;
	typeCountForColor[1][blockType]++;

	// 然后分析以前每回合的输入输出，并恢复状态
	// 循环中，color 表示当前这一行是 color 的行为
	// 平台保证所有输入都是合法输入
	for (int i = 1; i < turnID; i++)
	{
		int currTypeForColor[2] = { nextTypeForColor[0], nextTypeForColor[1] };
		int x, y, o;
		// 根据这些输入输出逐渐恢复状态到当前回合

		// 先读自己的输出，也就是自己的行为
		// 自己的输出是自己的最后一步
		// 然后模拟最后一步放置块
		cin >> blockType >> x >> y >> o;

		// 我当时把上一块落到了 x y o！
		Tetris myBlock(currTypeForColor[currBotColor], currBotColor);
		myBlock.set(x, y, o).place();

		// 我给对方什么块来着？
		typeCountForColor[enemyColor][blockType]++;
		nextTypeForColor[enemyColor] = blockType;

		// 然后读自己的输入，也就是对方的行为
		// 裁判给自己的输入是对方的最后一步
		cin >> blockType >> x >> y >> o;

		// 对方当时把上一块落到了 x y o！
		Tetris enemyBlock(currTypeForColor[enemyColor], enemyColor);
		enemyBlock.set(x, y, o).place();

		// 对方给我什么块来着？
		typeCountForColor[currBotColor][blockType]++;
		nextTypeForColor[currBotColor] = blockType;

		// 检查消去
		Util::eliminate(0);
		Util::eliminate(1);

		// 进行转移
		Util::transfer();
	}


	int blockForEnemy, finalX, finalY, finalO;

	// 做出决策（你只需修改以下部分）

	// 遇事不决先输出（平台上编译不会输出）
	Util::printField();

	// 贪心决策
	// 从下往上以各种姿态找到第一个位置，要求能够直着落下
	Tetris block(nextTypeForColor[currBotColor], currBotColor);
	
	double sum = 0;
	double sumpriority;
	int firstflag = 0;
	for (int y = 1; y <= MAPHEIGHT; y++)
		for (int x = 1; x <= MAPWIDTH; x++)
			for (int o = 0; o < 4; o++)
			{
			
				if (block.set(x, y, o).isValid() 
					//&& Util::checkDirectDropTo(currBotColor, block.blockType, x, y, o)
					&& iscanputblock(x ,y ,o, blockType, currBotColor)
					&& canfindway(x ,y ,o ,blockType, currBotColor)
					)
				{
					int tmp = cal(x, y, o, blockType, currBotColor);
					if (firstflag == 0 ){
						sum = tmp;
						finalX = x;
						finalY = y;
						finalO = o;
						firstflag = 1;
					}
					else
					if ( tmp == sum){
						double _priority;
						if (x <= MAPWIDTH / 2) _priority = 100 * (MAPWIDTH / 2 - x)  + o ;
						else _priority = 100 * ( x - MAPHEIGHT / 2 ) + 10 + o;
						if ( _priority > sumpriority){
							finalX = x;
							finalY = y;
							finalO = o;
							sumpriority = _priority;
						}
					}
					else
					if ( tmp > sum ){
						sum = tmp;
						finalX = x;
						finalY = y;
						finalO = o;
					}
				}
			}

//determined:
	// 再看看给对方什么好

	int maxCount = 0, minCount = 99;
	for (int i = 0; i < 7; i++)
	{
		if (typeCountForColor[enemyColor][i] > maxCount)
			maxCount = typeCountForColor[enemyColor][i];
		if (typeCountForColor[enemyColor][i] < minCount)
			minCount = typeCountForColor[enemyColor][i];
	}
	// if (maxCount - minCount == 2)
	// {
	// 	// 危险，找一个不是最大的块给对方吧
	// 	for (blockForEnemy = 0; blockForEnemy < 7; blockForEnemy++)
	// 		if (typeCountForColor[enemyColor][blockForEnemy] != maxCount)
	// 			break;
	// }
	// else
	// {
	// 	blockForEnemy = rand() % 7;
	// }
	double anssum = 1000000000;
	for (int i = 0; i < 7; i++) {
		Tetris tempblock( i, enemyColor);
		if(maxCount - minCount == 2 && typeCountForColor[enemyColor][i] == maxCount) continue;
		double sum = 0;
		int firstflag = 0;
		for (int y = 1; y <= MAPHEIGHT; y++)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++)
				{
				
					if ( tempblock.set(x, y, o).isValid() 
						//&& Util::checkDirectDropTo(enemyColor, i, x, y, o)
						&& iscanputblock(x ,y ,o, i, enemyColor)
						&& canfindway(x,y,o,i,enemyColor)
						)
					{
						int tmp = cal(x, y, o, i, enemyColor);
						if (firstflag == 0 ){
							sum = tmp;
							firstflag = 1;
						}
						else
						if ( tmp > sum )
						{
							sum = tmp;
						}
					}
				}
		if(anssum > sum) {
			anssum = sum; blockForEnemy = i;
		}
	}
	if(anssum == 1000000000) 
	{
		blockForEnemy = rand() % 7;
		while (maxCount == typeCountForColor[enemyColor][blockForEnemy]) {
			blockForEnemy = rand() % 7;
		} 
	}
		// 决策结束，输出结果（你只需修改以上部分）

	cout << blockForEnemy << " " << finalX << " " << finalY << " " << finalO;
	return 0;
}
