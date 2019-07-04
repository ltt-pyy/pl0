// 能生成中间代码的pl/0编译器

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "pl0.h"

// 第二步：错误处理程序
void error(long n){
    long i;

    printf(" ****");
	// 先输出一段空白字符，到达出错位置时停止
    for (i=1; i<=cc-1; i++){
		printf(" ");
    }
	// cc是指出错位置?
	// 报错提示信息，'^'指向出错位置，并提示错误类型号
    printf("^%2d\n",n);
    err++;
}

// 第三步：获取一个字符
void getch() {
	// ll代表行缓冲区长度，即输入的一行的长度
	// 这里表示：读完了一行(行指针与该行长度相等)
    if(cc==ll){
		// feof测试给定流 stream 的文件结束标识符
		if( feof(infile) ){
			// 如果到达了文件末尾，报错：程序不完整
			printf("************************************\n");
			printf("      program incomplete\n");
			printf("************************************\n");
			exit(1);
		}
		// 将行长度、行指针重置
		ll=0; cc=0;
		// 输出代码地址，宽度为4
		printf("%5d ", cx);
		// 当没有到行末时，输出读入的每一个字符，并且存储到line(代码数组中)去，同时行长度自增
		while( (!feof(infile)) && ((ch=getc(infile))!='\n') ){
			printf("%c",ch);
			ll=ll+1; line[ll]=ch;
		}
		// 换行
		printf("\n");
		// 填充 代码行数组的最后一个元素 为空格
		ll=ll+1; line[ll]=' ';
    }
	// 移动行缓冲指针，指向代码行的下一个字符
    cc=cc+1; ch=line[cc];
}

// 第四步：词法分析
void getsym(){
	// 声明计数变量
    long i,j,k;

	// 当前字符为' '或'\t'，继续获取下一个字符
    while(ch==' '||ch=='\t'){
		getch();
    }
	// 进行词法分析
	// 如果识别到字母，那么有可能是保留字或标识符
    if( isalpha(ch) ){
		// k用来计数，表示当前单词的长度
		k=0;
		do{
			// 如果长度 小于 最大的标识符长度，继续读入字符
			if(k<al){
				// a: 用来存储标识符的数组。a不断存储标识符的字符，并将计数增加
				a[k]=ch; k=k+1;
			}
			// 继续获取下一个字符
			getch();

		// 标识符首字符以后可以是 字母或数字
		} while( isalpha(ch)||isdigit(ch) );
		// 如果单词的计数 > kk(kk是所有标识符的最大长度，初始为10)
		if(k>=kk){
			kk=k;
		}else{
		// 否则将 存储标识符数组a 的k位后的字符 置为空格。这样做的意义是防止上一个标识符存在a中的内容影响到当前标识符，比如上一个标识符为“qwerty”，现在的标识符为“abcd”，如果不清后几位则a中会保存"abcdty"，这显然是错误的
			do{
				kk=kk-1; a[kk]=' ';
			} while(k<kk);
		}
		// id：最近读出的 标识符

		// 这里将a数组中的标识符字符串，复制到id数组中，为了与所有的保留字进行比较，以确定刚刚读出的单词是不是 保留字之一
		strcpy(id,a); i=0; j=norw-1;
		// 二分查找保留字表，将j设为保留字的最大数目
		do{
			// k作为二分查找的中间变量
			k=(i+j)/2;
			// 若当前单词小于或等于保留字表中的第k个。这里的判断依据的是字典序，那么可以推测符号表/（保留字数组）是按照字典序保存的
			if(strcmp(id,word[k])<=0){
				j=k-1;
			}
			// 若当前标识符大于或等于保留字表中的第k个
			if(strcmp(id,word[k])>=0){
				i=k+1;
			}
		// 查找结束条件为i>j
		} while(i<=j);

		// 找到了匹配的关键字
		if(i-1>j){
			// 将找到的保留字类型赋给sym(最近读出的 符号symbol类型)
			sym=wsym[k];
		}else{
		// 否则表示未找到匹配，则将sym置为ident类型，表示是标识符
			sym=ident;
		}
	// 如果首字符是数字，表示这个单词可能是 数字 类型
	} else if( isdigit(ch) ) { 
		// 这里的k用来记录数字的位数
		k=0;
		// num保存数字的值
		num=0;
		// 将标识符的类型设置为数字
		sym=number;
		do{
			// 将数字字符转换为数字，并拼接起来加到num上
			num = num*10+(ch-'0');
			// k++
			k = k+1;
			// 读入下一个字符
			getch();
		// 循环终止条件：读入的字符不是数字
		} while(isdigit(ch));
		// 如果大于数字的最大长度，报错：31号错误
		if(k>nmax){
			error(31);
		}
	// 如果首字符是':'，初步判断单词可能是":="，即becomes类型
	} else if(ch==':'){
		// 读入下一个字符判断是不是'='
		getch();
		// 如果是'='，将单词类型置为becomes，继续词法分析
		if(ch=='='){
			sym=becomes;
			getch();
		}else{
		// 否则，这个':'没有特殊含义
			sym=nul;
		}
	// 如果首字符是'<'，可能是: <=,<,<>，对应的类型为leq, lss, neq
	} else if(ch=='<'){
		getch();
		if(ch=='='){
			sym=leq;
			getch();
		} else if(ch == '>'){
			sym=neq;
			getch();
		} else {
			sym=lss;
		}
	// 如果首字符是'>'，可能是: >=,>，对应的类型为geq, gtr, 
	} else if(ch=='>'){
		getch();
		if(ch=='='){
			sym=geq; getch();
		}else{
			sym=gtr;
		}
	// 否则，去ASCII符号对应的单词类型表中查找
	} else{
		// 首先强制转换为unsigned char，再作为数组下标取得对应的值：比如+,-,*,/,nul之类
		sym=ssym[(unsigned char)ch];
		getch();
    }
}


// 生成并保存一行中间代码,x表示p-code指令, y,z是指令的两个操作数
void gen(enum fct x, long y, long z){
	// 如果当前生成代码的行数cx大于允许的最大长度cxmax
    if(cx > cxmax){
		// 输出报错信息，退出程序
		printf("program too long\n");
		exit(1);
    }
	// 如果没有超出,将目标代码cx存储到code数组中
	// instruction code[cxmax+1]是用来存储中间代码p-code的数组
    code[cx].f=x; code[cx].l=y; code[cx].a=z;
	// 将当前p-code代码行数加一
    cx=cx+1;
}

// 测试当前单词的合法性。用于错误语法的处理,若不合法则跳过单词值直到读到合法单词为止?
void test(unsigned long s1, unsigned long s2, long n){
	// 如果当前符号不在s1中
    if (!(sym & s1)) {
		// 报n号错误
		error(n);
		// 将s1赋值为s1和s2的并集
		s1=s1|s2;
		// 这个while的本质是pass掉所有不合法的符号,以恢复语法分析工作
		while(!(sym & s1)){
			// 获得下一个单词
			getsym();
		}
    }
}

// 将对象插入到符号表中
void enter(enum object k) {
	// tx是当前符号表指针，加一表示指向一个空表项
    tx=tx+1;
	// 改变tx序号对应表的内容
    strcpy(table[tx].name,id);	//name记录object k的id，从getsym获得
    table[tx].kind=k;			// kind记录k的类型,为传入参数
	// 根据类型不同进行不同的操作
    switch(k){
		// 常量：
		case constant:
			// 如果常量的数值大于约定的最大值
			if(num>amax){
				error(31);		// 报30号错误
				num = 0;		// num置0
			}
			// val保存该常量的值,结合上句可以看出,如果超过上限则保存0
			table[tx].val=num;
			break;
		// 变量
		case variable:
			table[tx].level=lev;	// 记录所属层次
			table[tx].addr=dx;		// 记录变量在当前层中的偏移地址
			dx=dx+1;				// 偏移量++,为下次插入做准备?
			break;
		// 过程
		case proc:
			table[tx].level=lev;	// 记录所属层次
			break;
    }
}

// 查找符号表的函数,参数id为需要寻找的符号
long position(char* id) {
    long i;
	// 把id放到符号表0号位置（哨兵?所以table的下标从1开始，到tx结束）
    strcpy(table[0].name,id);
	// 将i设置为符号表的最后一个位置,因为符号表是栈式结构,因此按层次逆序查找
    i=tx;
	// 如果当前表项的name和id不同
    while(strcmp(table[i].name,id)!=0){
		// 再向前找
		i=i-1;
    }
	// 返回为0，表示没找到；>0表示已找到
    return i;
}

// 处理常量声明的函数
void constdeclaration() {
	// 如果单词的类型是ident，说明是标识符
    if( sym==ident ){
		getsym();
		// 再获取下一个单词，如果是 等号 或者 赋值符号
		if( sym==eql || sym==becomes ){
			// 若是赋值符号，报1号错误,因为声明应该使用等号
			if(sym==becomes){
				error(1);
			}
			// 获得下一个单词
			getsym();
			// 如果 单词的类型 是数字
			if(sym==number){
				// 插入这个常量到符号管理表中
				enter(constant);
				// 获得下一单词
				getsym();
			} else {
				// 如果等号后面不是 数字， 就报2号错误
				error(2);
			}
		} else {
			// 如果常量标识符const后面不是 等号 或者 赋值符号，报3号错误
			error(3);
		}
    }
	// 如果常量声明的第一个单词不是标识符，报4号错误 (当然前面已经识别了const，所以第一个单词应该是标识符)
	else {
		error(4);
    }
	// 常量声明结束
}

// 处理变量声明的函数
void vardeclaration() {
	// 变量声明的语法比常量声明的语法要简单一点，可以直接var a，所以只要识别出变量声明符后面的单词是标识符就可以了。
    if( sym==ident ){
		// 如果单词的类型是 标识符，则将该变量插入到符号表中
		enter(variable); 
		// 获得下一个单词
		getsym();
    } else {
		// 如果变量声明符后面的单词不是标识符，则报4号错误
		error(4);
    }
	// 变量声明结束
}

// 列出中间代码p-code的函数 list code generated for this block
void listcode(long cx0) {
    long i;
	// 格式化输出,分别输出序号,p-code指令的助记符,层次,地址.实际的输出效果和我们实际的p-code相同
    for(i=cx0; i<=cx-1; i++){
		printf("%10d%5s%3d%5d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
    }
}

// 
void expression(unsigned long);

// 处理因子的程序(计算数、变量、括号)
void factor(unsigned long fsys){
	// 定义参数
    long i;
	// 测试单词的合法性,判别当前sym是否在facbegsys中,后者在main中定义,如果不在报24号错误。说明不是因子?
	// facbegsys=ident|number|lparen，以 标识符、数字、左括号 开始
    test(facbegsys,fsys,24);
	
	// 循环处理因子
    while(sym & facbegsys){
		// 如果识别到标识符
		if(sym==ident){
			// 查表,记录其在符号表中的位置,保存至i
			i=position(id);
			// 如果返回值为0，则表示符号表中不存在这个名字的标识符，报11号错
			if(i==0){
				error(11);
			} else {
				// 如果在符号表中找到了对应的符号项，则根据不同的符号类型进行中间代码的产生
				switch( table[i].kind ) {
					// 如果是常量类型,生成lit指令,操作数为0,val
					case constant:
						gen(lit,0,table[i].val);
						break;
					case variable:
					// 如果是变量类型,生成lod指令,操作数为lev-level,addr
						gen(lod,lev-table[i].level,table[i].addr);
						break;
					// 如果因子处理中识别到了过程标识符,报21号错误?
					case proc:
						error(21);
						break;
				}
			}
			// 获取下一个单词
			getsym();
		} else if(sym==number){
		// 如果识别到数字
			if(num>amax){
				// 判别数字是否超过规定上限,超过上限,报31号错误
				error(31);
				// 将数字重置为0
				num=0;
			}
			// 生成lit指令,将num的值放到栈顶
			gen(lit,0,num);
			// 获取下一个单词
			getsym();

		// 如果识别到左括号
		} else if( sym==lparen ){
			// 获取下一个单词
			getsym();
			// 调用表达式的处理程序来处理,递归下降子程序方法
			expression(rparen|fsys);
			// 如果识别到右括号
			if(sym==rparen){
				// 获取下一个单词
				getsym();
			} else {
				// 否则报22号错误
				error(22);
			}
		}
		// 测试结合是否在fsys中,若不是,抛出23号错误?
		test(fsys,lparen,23);
    }
}

// 项的分析程序（* /操作）（我觉得* /是复杂一点的表达式）
void term(unsigned long fsys){
	// 项的分析过程开始 multiply operator
    unsigned long mulop;
	// 项的第一个符号应该是因子,调用因子分析程序
    factor(fsys|times|slash);
	// 如果因子后面是乘/除号
    while( sym==times||sym==slash ){
		// 使用mulop保存当前的运算符
		mulop=sym;
		// 获取下一个单词
		getsym();
		// 调用因子分析程序分析运算符后的因子
		factor(fsys|times|slash);
		// 如果运算符是乘号
		if( mulop==times ){
			// 生成opr指令,乘法指令
			gen(opr,0,4);
		} else {
			// 生成opr指令,除法指令
			gen(opr,0,5);
		}
    }
}

// 表达式的分析程序 (+ -操作) （+ -是简单的表达式）
void expression(unsigned long fsys){
	// addop: add operator
    unsigned long addop;
	// 如果表达式的第一个符号是+/-符号
    if( sym==plus || sym==minus ){
		// 保存当前单词(运算符)
		addop=sym;
		// 下一个单词
		getsym();
		// 正负号后面接项,调用项的分析过程
		term(fsys|plus|minus);
		// 如果负号开头：比如-mul1*mul2
		if(addop==minus) {
			// 生成opr指令,完成取反运算
			gen(opr,0,1);
		}
    } else {
		// 如果不是符号开头,直接调用项的分析过程
		term(fsys|plus|minus);
    }
	// 项后面可以接若干个term,使用操作符+-相连,因此此处用while
    while(sym==plus||sym==minus){
		// 记录运算符类型
		addop=sym;
		// 获取下一个sym类型
		getsym();
		// 调用项的分析过程
		term(fsys|plus|minus);
		if(addop==plus){
			// 生成opr指令,完成加法运算
			gen(opr,0,2);
		} else {
			// 否则生成减法指令
			gen(opr,0,3);
		}
    }
}

// 条件分析程序 relop: relational operator
void condition(unsigned long fsys){
	// 关系运算符保存在relop中
    unsigned long relop;

    // 如果单词类型为 odd 
    if( sym==oddsym ){
    	// 获取下一个单词
		getsym();
		// odd里面可以接受表达式，所以调用表达式分析函数
		expression(fsys);
		// 生成opr 6号指令,完成奇偶判断运算
		gen(opr,0,6);

	// 如果不是odd，则调用表达式分析过程对表达式进行计算
    } else {
    	// 调用表达式分析函数对表达式进行计算
		expression(fsys|eql|neq|lss|gtr|leq|geq);

		// 如果存在关系运算符集合之外的符号
		if( !(sym & (eql|neq|lss|gtr|leq|geq)) ){
			// 报20号错误
			error(20);
		} else {
			// 记录当前关系运算符号类型
			relop=sym;
			// 获取下一个单词，关系运算符后面跟的肯定是表达式
			getsym();
			// 调用表达式分析过程对表达式进行分析
			expression(fsys);
			// 根据当前符号类型不同完成不同的操作
			switch(relop){
				// 如果是等号,生成opr 8号指令,判断是否相等
				case eql:
					gen(opr,0,8);
					break;
				// 如果是不等号,生成opr 9号指令,判断是否不等
				case neq:
					gen(opr,0,9);
					break;
				// 如果是小于号,生成opr10号指令,判断是否小于
				case lss:
					gen(opr,0,10);
					break;
				// 如果是大于等于号,生成opr11号指令,判断是否大于等于
				case geq:
					gen(opr,0,11);
					break;
				// 如果是大于号,生成opr12号指令,判断是否大于
				case gtr:
					gen(opr,0,12);
					break;
				// 如果是小于等于号,生成opr13号指令,判断是否小于等于
				case leq:
					gen(opr,0,13);
					break;
			}
		}
    }
}

// 语句处理
void statement(unsigned long fsys){
    
    long i,cx1,cx2;

    // 如果以标识符开始
    if(sym==ident) {
    	// 根据标识符调用position查找它在符号表中的位置
		i=position(id);
		// 如果i=0，表示没有找到，报11号错误
		if(i==0){
		    error(11);
		// 如果在符号表中找到了该符号,但该符号的类型不是变量,那么现在的操作属于给非变量赋值，报12号错误，将符号表标号i置零
		} else if(table[i].kind != variable) {
		    error(12);
		    i=0;
		}
		// 获取下一个单词
		getsym();
		// 如果单词类型是赋值符号；如果读到的不是赋值符号,报13号错误
		if(sym==becomes){
			// 获取下一个单词
		    getsym();
		} else {
		    error(13);
		}
		// 赋值符号:=的后面可以跟表达式,因此调用表达式处理子程序
		expression(fsys);
		// 如果符号表中找到了合法的符号
		if(i!=0){
			// 生成一条sto指令用来将表达式的值写入到相应变量的地址
		    gen(sto,lev-table[i].level,table[i].addr);
		}

	// 如果读到的符号是call关键字
    } else if(sym==callsym) {
    	// 获取下一个单词
		getsym();
		// 如果call后面的单词不是 标识符，报14号错误
		if(sym!=ident) {
		    error(14);
		} else {
		// call后的是标识符，通过标识符的名字在符号表中进行查找，并将结果返回给i
		    i=position(id);
		    // 如果i=0，表示没有找到，报11号错误
		    if(i==0) {
				error(11);
			// 如果找到了对应的符号项，而且它的类型是 proc，代表它是过程的标识符
		    } else if(table[i].kind==proc){
		    	// 生成cal指令来实现call操作
				gen(cal,lev-table[i].level,table[i].addr);
		    } else {
		    	// 如果种类不为过程类型,报15号错误
				error(15);
	    	}
	    	// 获取下一个单词
	    	getsym();
		}
	// 如果读到的符号是if关键字	
    } else if( sym==ifsym ) {
		getsym();
		// if后面跟的应该是条件语句,调用条件分析过程
		condition(fsys|thensym|dosym);
		// 如果条件语句后面跟的是then关键字
		if(sym==thensym) {
	    	// 获取下一个单词
		    getsym();
		} else {
			// 如果条件语句后面接的不是then,报16号错误
		    error(16);
		}
		// code index：中间代码的索引，记录当前的生成代码位置到cx1
		cx1=cx;
		// 生成条件跳转指令,跳转位置暂填0
		gen(jpc,0,0);
		// 分析then关键字后面的语句
		statement(fsys);
		// 将之前记录的代码的位移地址改写到现在的生成代码位置?
		code[cx1].a=cx;	

	// 如果读入的单词为begin关键字
    } else if(sym==beginsym) {
    	// 获取下一个单词
		getsym();
		// begin后面默认接语句,调用语句分析程序
		statement(fsys|semicolon|endsym);
		// 如果当前的单词是分号，或者是一个语句，继续分析
		while( sym==semicolon || (sym&statbegsys) ){
			// 如果单词是分号
		    if(sym==semicolon){
		    	// 获取下一个单词
				getsym();
		    } else {
		    	// 否则报10号错误
				error(10);
		    }
		    // 继续进行语句的分析
		    statement(fsys|semicolon|endsym);
		}
		// 如果当前的单词是end关键词
		if(sym==endsym){
			// 获取下一个单词
		    getsym();
		}else{
			// 如果statement语法块不是以end结束，报17号错误
		    error(17);
		}
	// 如果当前的单词是while关键词
    } else if(sym==whilesym) {
    	// 记录分析while开始时当前生成代码的位置
		cx1=cx;
		// 获取下一个单词
		getsym();
		// while后面跟的默认是循环条件，需要调用条件语句的分析过程(while... do...)
		condition(fsys|dosym);
		// 记录在分析完条件之后的生成代码的位置,也是do开始的位置
		cx2=cx;
		// 生成一个条件跳转指令,但是跳转位置(a)置零?
		gen(jpc,0,0);
		// 条件后应该接do关键字，否则报18号错误
		if(sym==dosym) {
			// 获取下一个单词
		    getsym();
		} else {
		    error(18);
		}
		// 分析处理循环部分的语句
		statement(fsys);
		// 执行完一次循环语句后，需要跳转到循环条件进行判断，所以需要生成无条件跳转指令
		gen(jmp,0,cx1);
		// 给之前生成的跳转指令设定跳转的位置为当前位置?
		code[cx2].a=cx;
    }
    // 测试当前字符是否合法,如果没有出现在fsys中,报19号错
    test(fsys,0,19);
}
		
// 	进行语法分析的主程序,lev表示语法分析所在层次,tx是当前符号表指针,fsys是用来恢复错误的单词集合
/*
long dx;		// 数据地址索引
long lev;		// 语法分析所在层次
long tx;		// tx是当前符号表指针
*/
void block(unsigned long fsys){
    long tx0;		// 初始符号表索引
    long cx0; 		// 初始中间代码索引
    long tx1;		// 在处理嵌套的过程时，先要保存当前符号表的索引
    long dx1;		// 数据地址索引

	// 记录运行栈空间的栈顶位置,设置为3是因为需要预留SL,DL,RA的空间（3个寄存器）
    dx=3;
	// 记录当前符号表的栈顶位置
	tx0=tx;
	// 符号表当前位置的偏移地址记录下一条生成代码开始的位置
	table[tx].addr=cx;
	// 产生一条jmp类型的无条件跳转指令,跳转位置未知?
	gen(jmp,0,0);
	// 如果过程所处的层次大于允许的最大嵌套层次，报32号错误
    if(lev>levmax){
		error(32);
    }
	// 循环开始，先处理常量声明
    do{
		// 如果单词类型是const，常量声明
		if(sym == constsym){
			// 获取下一个单词
			getsym();
			// 循环开始
			do{
				// 在const后，需要处理常量声明
				constdeclaration();
				// 如果声明常量后面接的是逗号，说明常量声明没有结束，进入下一循环，直到后面跟的不是','
				while( sym==comma ) {
					getsym();
					constdeclaration();
				}
				// 如果读到了分号,说明常量声明已经结束了
				if(sym==semicolon){
					getsym();
				} else {
				// 如果没有分号，说明声明语句没有正常结束，报5号错误 
					error(5);
				}
			// 循环直到遇到下一个不是 标识符
			} while(sym==ident);
		}
	
		// 如果 单词类型是var保留字
		if( sym==varsym ){
			// 获取下一个单词
		    getsym();
			// 循环开始，处理这一var语句中所有的变量声明
		    do{
				// 在var后，处理变量声明
				vardeclaration();
				// 如果读到了逗号,说明声明未结束,进入循环，直到后面跟的单词类型不是','
				while( sym==comma ) {
					// 获取下一个单词
					getsym();
					// 处理变量声明
					vardeclaration();
				}
				// 如果读到了分号,说明变量声明已经结束了
				if(sym==semicolon) {
					getsym();
				} else {
				// 如果没有分号，说明声明语句没有正常结束，报5号错误 
					error(5);
				}
			// 循环直到遇到下一个不是 标识符
		    } while(sym==ident);
		}
		
		// 如果 单词类型是procedure保留字
		while( sym==procsym ){
			// 获取下一个单词
		    getsym();
			// 第一个符号应该是标识符类型，即代表过程的标识符
		    if(sym==ident) {
				// 将该符号录入符号表,类型为过程,因为跟在proc后面的一定是过程名
				enter(proc);
				// 获取下一个单词
				getsym();
		    } else {
				// 如果第一个符号不是标识符类型,报4号错误
				error(4);
		    }
			// 如果读到了分号,说明 过程声明结束
		    if(sym==semicolon){
				// 获取下一单词
				getsym();
		    } else {
				// 如果声明过程之后没有跟分号,语句没有正常结束,报5号错误
				error(5);
		    }
			// 执行分程序的分析过程：语法分析所在层次lev++表示进入下一层；tx1是在处理嵌套的过程时，先要保存当前符号表的索引
		    lev=lev+1; tx1=tx; dx1=dx;
			// 执行分程序的分析过程
		    block(fsys|semicolon);
		    // 递归返回后，各个变量恢复原来的值
		    lev=lev-1; tx=tx1; dx=dx1;
		    // 递归调用返回后应该接分号。如果是分号
		    if(sym == semicolon) {
		    	// 获取下一个单词
				getsym();
				// 测试当前单词的合法性。用于错误语法的处理（6号错误）
				test(statbegsys|ident|procsym,fsys,6);
		    } else {
		    	// 如果接的不是分号，报5号错误
				error(5);
		    }
		}
		// 测试当前的sym是否合法。报7号错误
		test(statbegsys|ident,declbegsys,7);
	// 一直循环到sym不在声明符号集中为止
    } while(sym & declbegsys);

    // 将之前生成无条件跳转指令的目标地址指向当前位置
    code[table[tx0].addr].a=cx;
    // 记录当前代码的分配开始索引
    table[tx0].addr=cx;
    // 记录当前代码分配的地址
    cx0=cx;
    // 生成int指令,分配dx个空间
    gen(Int,0,dx);
    // 调用语法分析程序
    statement(fsys|semicolon|endsym);
    // 生成0号opr程序,完成返回操作
    gen(opr,0,0);
    // 测试当前状态是否合法,有问题报8号错误
    test(fsys,0,8);
    // 列出该block所生成的p-code
    listcode(cx0);
}

// 计算基地址的函数
long base(long b, long l){
	// 声明计数变量
    long b1;
	// 记录当前层的基地址
    b1=b;
	// 如果层数l大于0,即寻找的不是本层
    while (l>0) {
		// 记录当前层数据基址的内容
		b1=s[b1];
		// l--
		l=l-1;
    }
	// 将找到的基地址返回
    return b1;
}

// 解释执行程序
void interpret(){
	// 设置三个寄存器,分别记录下一条指令,基址地址和栈顶指针
    long p,b,t;
    // 指令寄存器,类型为instruction,为了存放当前指令
    instruction i;

    // 输出程序开始运行的提示语句
    printf("start PL/0\n");
    // 将指令寄存器置零；将基址地址置为1；将栈顶指针置零；
    p=0; b=1; t=0; 

    // 将数据栈的第一层置零,对应SL
    s[1]=0;
    // 将数据栈的第二层置零,对应DL
    s[2]=0;
    // 将数据栈的第三层置零,对应RA
    s[3]=0;

    // 开始解释执行
    do{
    	// 获取当前需要执行的代码
		i=code[p];
		// 将指令寄存器+1,以指向下一条指令
		p=p+1;
		// 针对当前指令，对 操作码的类型 执行不同操作
		switch(i.f){
			// lit
		    case lit:
		    	// 栈顶指针加1，将a操作数的值放入栈顶
				t=t+1; s[t]=i.a;
				break;
			// 针对opr类型的指令
		    case opr:
				switch(i.a){
			    	// 0对应return操作
				    case 0:
						// t取到该层数据栈SL-1的位置,意味着将该层的数据栈全部清空
						t=b-1;
						// 将指令指针指向RA的值,即获得return address
						p=s[t+3];
						// 将基址指针指向DL的值,即获得了return之后的基址,因为被调用层次的DL指向调用层次的基址
						b=s[t+2];
						break;

					// 1对应取反操作
				    case 1:
						s[t]=-s[t];
						break;

					// 2对应求和操作
				    case 2:
					    // 栈顶指针退一格，将栈顶和次栈顶中的数值求和放入新的栈顶,注意运算后的栈顶是下降一格的,下面的运算亦如此
						t=t-1; s[t]=s[t]+s[t+1];
						break;

					// 3对应减操作
				    case 3:
						t=t-1; s[t]=s[t]-s[t+1];
						break;

					// 4对应乘积操作
				    case 4:
						t=t-1; s[t]=s[t]*s[t+1];
						break;
					// 5对应相除
				    case 5:
						t=t-1; s[t]=s[t]/s[t+1];
						break;
					// 6对应判断是否栈顶数值为奇数
				    case 6:
						s[t]=s[t]%2;
						break;
					
					// 8号对应等值判断
				    case 8:
						t=t-1; s[t]=(s[t]==s[t+1]);
						break;

					// 9号对应不等判断
				    case 9:
						t=t-1; s[t]=(s[t]!=s[t+1]);
						break;
				    
				    // 10号对应小于判断
				    case 10:
						t=t-1; s[t]=(s[t]<s[t+1]);
						break;
				    
					// 11号对应大于等于判断
				    case 11:
						t=t-1; s[t]=(s[t]>=s[t+1]);
						break;
				    
					// 12号对应着大于判断
				    case 12:
						t=t-1; s[t]=(s[t]>s[t+1]);
						break;
				    
					// 13号对应着小于等于判断
				    case 13:
						t=t-1; s[t]=(s[t]<=s[t+1]);
				}
				break;
		    
			// 如果是lod指令
		    case lod:
			    // 栈顶指针指向新栈
				t=t+1;
				// 将与当前数据层层次差为l,层内偏移为a的栈中的数据存到栈顶
				s[t]=s[base(b,i.l)+i.a];
				break;

			// sto指令
		    case sto:
		    	// 将当前栈顶的数据保存到与当前层层差为l,层内偏移为a的数据栈中
				s[base(b,i.l)+i.a]=s[t];
				printf("%10d\n", s[t]); t=t-1;
				break;

			// generate new block mark
		    case cal:
		    	// 由于要生成新的block,因此栈顶压入SL的值
				s[t+1]=base(b,i.l);
				// 在SL之上压入当前数据区的基址,作为DL
				s[t+2]=b;
				// 在DL之上压入指令指针,即是指令的断点,作为RA
				s[t+3]=p;
				// 把当前的数据区基址指向新的SL
				b=t+1;
				// 从a的位置继续执行程序,a来自instruction结构体
				p=i.a;
				break;

			// 	对Int指令,将栈顶指针上移a个位置
		    case Int:
				t=t+i.a;
				break;

			// 对jmp指令,将指令指针指向a
		    case jmp:
				p=i.a;
				break;

			// 对于jpc指令
		    case jpc:
		    	// 如果栈顶数据为零，则将指令指针指向a
				if(s[t]==0){
				    p=i.a;
				}
				// 栈顶向下移动
				t=t-1;
		}
    } while (p!=0);

    // p-code执行结束
    printf("end PL/0\n");
}


// 第一步：先看一下main函数，大致了解程序运行的过程
main(){
	// i代表ASCII码的值: 0~255
    long i;
	// 为每个ASCII字符赋值，值是指字符对应的 symbol类型，比如ssym['+']=plus;
	// 上一句话中的symbol类型，我认为是 一些特殊的ASCII字符表示的语义性的东西，比如加减乘除，其他没有特殊含义的ASCII字符，他们的symbol类型，就赋值为nul
    for(i=0; i<256; i++){
		// nul表示这个字符并无特殊含义，先这样全部赋值为nul，之后再进行其他操作
		ssym[i]=nul;
    }
	
	
	// word[norw][al+1];
	// 这一段代码初始化的是 保留字数组，存储量相应的 保留字 (11个)
    strcpy(word[0],  "begin     ");
    strcpy(word[1],  "call      ");
    strcpy(word[2],  "const     ");
    strcpy(word[3],  "do        ");
    strcpy(word[4],  "end       ");
    strcpy(word[5],  "if        ");
    strcpy(word[6],  "odd       ");
    strcpy(word[7],  "procedure ");
    strcpy(word[8],  "then      ");
    strcpy(word[9],  "var       ");
    strcpy(word[10], "while     ");
	
	// 保留字表中每个保留字对应的symbol类型 (保留字)
    wsym[0]=beginsym;
    wsym[1]=callsym;
    wsym[2]=constsym;
    wsym[3]=dosym;
    wsym[4]=endsym;
    wsym[5]=ifsym;
    wsym[6]=oddsym;
    wsym[7]=procsym;
    wsym[8]=thensym;
    wsym[9]=varsym;
    wsym[10]=whilesym;
	
	// ssym[256]
	// 一些特殊ASCII字符对应的symbol类型需要修改：+-*/()=,.;
    ssym['+']=plus;
    ssym['-']=minus;
    ssym['*']=times;
    ssym['/']=slash;
    ssym['(']=lparen;
    ssym[')']=rparen;
    ssym['=']=eql;
    ssym[',']=comma;
    ssym['.']=period;
    ssym[';']=semicolon;
	
	// char mnemonic[8][3+1]
	// 初始化p-code指令助记符数组
    strcpy(mnemonic[lit],"lit");
    strcpy(mnemonic[opr],"opr");
    strcpy(mnemonic[lod],"lod");
    strcpy(mnemonic[sto],"sto");
    strcpy(mnemonic[cal],"cal");
    strcpy(mnemonic[Int],"int");
    strcpy(mnemonic[jmp],"jmp");
    strcpy(mnemonic[jpc],"jpc");
	
	// 声明开始 表达式开始 项开始 的符号集合
	// 这里的|运算符，执行按位或运算，只要与其中一个进行或运算不为0，则条件成立，代表是某个语法元素的开始
	// 以 const、var、procedure 开始
    declbegsys=constsym|varsym|procsym;
	// 以 begin、call、if、while 开始
    statbegsys=beginsym|callsym|ifsym|whilesym;
	// 以 标识符、数字、左括号 开始
    facbegsys=ident|number|lparen;


	/* 主程序真正开始执行，前面是一些初始化 */

	// 提示用户输入源码的地址
    printf("please input source program file name: ");
	// 读入用户输入的文件路径到 infilename
    scanf("%s",infilename);
	// 输入换行符\n
    printf("\n");
	
	// fopen: 如果文件打开成功返回一个 FILE 指针。否则返回 NULL，且设置全局变量 errno 来标识错误
    if( (infile = fopen(infilename,"r")) == NULL ){
		printf("File %s can't be opened.\n", infilename);
		exit(1);
    }
    
	// 开始编译前的准备工作（此时已经读入源文件了，所以与前面说的初始化过程不一样）
    err=0;		// 将错误号置为0
    cc=0;		// 行缓冲指针指向0
	cx=0;		// 中间代码的行数置为0
	ll=0;		// 词法分析行缓冲区长度置零 (读入的字符串?)
	ch=' ';		// 当前读入的字符设为' '
	kk=al;		// kk的值初始化为al，等于10
	
	// 词法分析程序，将字符流转换为 单词，并返回
	getsym();

    lev=0;		// 语法分析所在层次
	tx=0;		// tx是指向当前符号表指针(index，就是索引的那种)
	
	// 语法分析程序 (块)
    block(declbegsys|statbegsys|period);
	
	// 如果符号不是句号，产生9号错误，即：程序必须以'.'结束
    if(sym != period) {
		error(9);
    }
	// 如果err为0表示没有错误，就进行下一步
    if(err==0) {
		// 解释程序,开始解释执行生成的p-code代码
		interpret();
    }
	else {
		// 否则pl/0程序中有错误，报错
		printf("errors in PL/0 program\n");
    }
	// 编译结束，关闭文件
    fclose(infile);
}

