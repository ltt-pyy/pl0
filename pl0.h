#include <stdio.h>

/*
no.		代表	数目
max.	代表	最大


pl/0 已有的语法:
	变量类型、函数定义
		const var procedure

	程序结构
		begin end
		if then
		while do
		call procedure
	
*/

// 宏定义
#define norw       11             // 保留字的数目
#define txmax      100            // 符号表的长度
#define nmax       14             // 数字的最大长度
#define al         10             // 标识符的最大长度
#define amax       2047           // 相对地址的最大值0-2048
#define levmax     3              // 最大嵌套层数
#define cxmax      2000           // 生成目标代码p-code数组的最大长度

#define nul	   0x1					// ASCII字符默认的symbol类型，说明他不是特殊的字符/字符串结束符?
#define ident      0x2				// 字母开头的数字串
#define number     0x4				// 无符号数，若是实数，则采用小数点前后都有非空数字串的形式

/* 这一块是一些宏定义，代表symbol类型 */
// 运算符
#define plus       0x8				// +
#define minus      0x10				// -
#define times      0x20				// *
#define slash      0x40				// /
#define eql        0x100			// =
#define neq        0x200			// <>
#define lss        0x400			// <
#define leq        0x800			// <=
#define gtr        0x1000			// >
#define geq        0x2000			// >=
#define lparen     0x4000			// (
#define rparen     0x8000			// )
#define comma      0x10000			// ,
#define semicolon  0x20000			// ;
#define period     0x40000			// .

#define becomes    0x80000			// := 赋值符号

// 保留字 11 个 对应wsym数组
#define beginsym   0x100000			// begin
#define endsym     0x200000			// end
#define ifsym      0x400000			// if
#define oddsym     0x80				// 内置函数odd()，也算作保留字
#define thensym    0x800000			// then
#define whilesym   0x1000000		// while
#define dosym      0x2000000		// do
#define callsym    0x4000000		// call
#define constsym   0x8000000		// const
#define varsym     0x10000000		// var
#define procsym    0x20000000		// procedure


// 对象的类型包括三种：常量、变量、过程
enum object {
    constant, variable, proc
};


// 中间代码p-code的指令类型，共 8 种
enum fct {
    lit, 
	opr, 
	lod, 
	sto, 
	cal, 
	Int, 
	jmp, 
	jpc, 
	/*
	red, 
	wrt
	*/
};

// 定义代表p-code指令的结构体
typedef struct{
    enum fct f;		// 操作码 (fct中的一个)
    long l; 		// 嵌套层次 (标识符引用层减去定义层)
    long a; 		// 偏移地址/... （不同的指令含义不同）
} instruction;


/* 
	lit 0, a : 取常量a放到数据栈栈顶
    opr 0, a : 执行运算，a表示执行何种运算(+ - * /)
    lod l, a : 取变量放到数据栈栈顶(相对地址为a,层次差为l)
    sto l, a : 将数据栈栈顶内容存入变量(相对地址为a,层次差为l)
    cal l, a : 调用过程(入口指令地址为a,层次差为l)
    Int 0, a : 数据栈栈顶指针增加a
    jmp 0, a : 无条件转移到指令地址a
    jpc 0, a : 条件转移到指令地址a
	red l, a : 读数据并存入变量，
    wrt 0, 0 : 将栈顶内容输出
	
	opr 0, 0 : 过程调用结束后,返回调用点并退栈
	opr 0, 1 : 栈顶元素取反 (~)
	opr 0, 2 : 次栈顶与栈顶相加，退两个栈元素，结果值进栈 (相当于+?)
	opr 0, 3 : 次栈顶减去栈顶，退两个栈元素，结果值进栈 (-)
	opr 0, 4 : 次栈顶乘以栈顶，退两个栈元素，结果值进栈 (*)
	opr 0, 5 : 次栈顶除以栈顶，退两个栈元素，结果值进栈 (/)
	opr 0, 6 : 栈顶元素的奇偶判断，结果值在栈顶 (odd())
	opr 0, 7 : 
	opr 0, 8 : 次栈顶与栈顶是否相等，退两个栈元素，结果值进栈 (==)
	opr 0, 9 : 次栈顶与栈顶是否不等，退两个栈元素，结果值进栈 (!=)
	opr 0, 10 : 次栈顶是否小于栈顶，退两个栈元素，结果值进栈 (<)
	opr 0, 11 : 次栈顶是否大于等于栈顶，退两个栈元素，结果值进栈 (>=)
	opr 0, 12 : 次栈顶是否大于栈顶，退两个栈元素，结果值进栈 (>)
	opr 0, 13 : 次栈顶是否小于等于栈顶，退两个栈元素，结果值进栈 (<=)

	opr 0, 14 : 栈顶值输出至屏幕
	opr 0, 15 : 屏幕输出换行
	opr 0, 16 : 从命令行读入一个输入置于栈顶
*/

// 全局变量定义
char ch;               // 最近读出的 一个字符
unsigned long sym;     // 最近读出的 符号symbol/单词 的类型
char id[al+1];         // 最近读出的 标识符
long num;              // 最近读出的 数字
long cc;               // 行缓冲指针(index)
long ll;               // 行缓冲区长度(length)
long kk, err;		   // 用于错误处理程序定义的变量( err：错误号; kk：/可能用于识别标识符长度大于10等出错情况? ) 
long cx;               // 中间代码指针(起到index的作用)?

char line[81];				// 缓冲一行代码
char a[al+1];				// 用来存储标识符的数组
instruction code[cxmax+1];	// 用来存储中间代码p-code的数组，最大容量为 cxmax+1
char word[norw][al+1];		// 存储保留词的字符串数组，第一维为关键词的字符数组，第二维度为字符串数组

unsigned long wsym[norw];	// 保留字表中每个保留字对应的symbol类型
unsigned long ssym[256];	// ASCII符号对应的symbol类型?


// mnemonic [n?'mɑn?k] p-code指令的助记符数组
char mnemonic[8][3+1];

// 代表 声明开始、语句开始、项开始(表达式) 的符号集合
unsigned long declbegsys, statbegsys, facbegsys;

// 符号表数组
struct{
    char name[al+1];	// 符号元素的名字
    enum object kind;	// 根据符号symbol的类型保存相应的信息
    long val;			// 如果是常量，val中保存常量的值
    long level;			// 如果是变量或过程，保存存放层数和 （中间代码）开始地址的偏移地址
    long addr;
} table[txmax+1];

// pl0文件的路径
char infilename[80];
// pl0文件指针
FILE* infile;

// 语法分析中用到的变量
long dx;		// 数据地址索引
long lev;		// 语法分析所在层次
long tx;		// tx是当前符号表指针（索引）

// 为解释程序 预留 空间（一个数据栈）
#define stacksize 50000
long s[stacksize];	// datastore

