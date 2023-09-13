//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include <reg51.h>
#include <intrins.h>

//-----------------------------------------------------------------
// 数据类型宏定义
//-----------------------------------------------------------------
#define uchar unsigned char
#define uint  unsigned int

//-----------------------------------------------------------------
// 定义LCD使用的IO口
//-----------------------------------------------------------------
sbit ep=P2^0;  //使能信号端口
sbit rw=P2^1;  //读写选择端口
sbit rs=P2^2;  //寄存器选择端口

//-----------------------------------------------------------------
// 定义按键、LED、蜂鸣器等使用的IO口
//-----------------------------------------------------------------
sbit KEY1=P0^0;  			//摇床按键设置
sbit KEY2=P0^1;  			//音乐按键设置
sbit KEY3=P0^2;  			//亮灯/睡眠按键设置
sbit LED1=P0^3;				//声音异常设置
sbit LED2=P0^4;				//温度异常设置
sbit LED3=P0^5;				//湿度异常设置
sbit BUZZER=P2^3;  		//蜂鸣器设置


//-----------------------------------------------------------------
// 定义温度传感器DS18B20数据IO口
//-----------------------------------------------------------------
sbit DQ = P0^6;  
uchar FLAG=1;   //正负温度标志
unsigned char TMPH,TMPL;

//-----------------------------------------------------------------
// 定义湿度传感器DHT11数据IO口
//-----------------------------------------------------------------
sbit  Data=P0^7;       //温湿度IO
signed char  humi_value;//湿度

//-----------------------------------------------------------------
// 定义ADC0832数据口
//-----------------------------------------------------------------
sbit cs = P3^0;//片选使能，低电平有效
sbit clk = P3^2;//芯片时钟输入
sbit dio = P3^3;//数据输入DI与输出DO
signed char  noise_value;//噪音分贝

//-----------------------------------------------------------------
// 定义继电器控制数据IO口
//-----------------------------------------------------------------
sbit LIGHT = P2^4; //控制亮灯/睡眠IO口

//-----------------------------------------------------------------
// 定义步进电机控制数据IO口
//-----------------------------------------------------------------
#define MotorDriver P3  	//步进电机驱动接口

//-----------------------------------------------------------------
// 定义一些错误标志
//-----------------------------------------------------------------
unsigned char flag1;//温度过低
unsigned char flag2;//温度过高
unsigned char flag3;//湿度过高
unsigned char flag4;//噪音过高
unsigned char flag5;//是否摇床标志
unsigned char send_flag1;//发送婴儿床温度过高
unsigned char send_flag2;//发送婴儿床温度过低
unsigned char send_flag3;//发送婴儿尿床
unsigned char send_flag4;//发送婴儿哭闹

//-----------------------------------------------------------------
// 定义代码格式的音乐
//-----------------------------------------------------------------
unsigned char code SONG_TONE[]=
{
 	212,212,190,212,159,169,212,212,190,212,142,159,212,212,106,126,129,169,190,119,119,126,159,142,159,0
};
unsigned char code SONG_LONG[]=
{
 	9,3,12,12,12,24,9,3,12,12,12,24,9,3,12,12,12,12,12,9,3,12,12,12,24,0
};

//-----------------------------------------------------------------
// 定义步进电机用的变量
//-----------------------------------------------------------------
//输出励磁序列的频率参数{TH1,TL1}
const uchar Timer[9][2]={{0xDE,0xE4},{0xE1,0xEC},{0xE5,0xD4},{0xE9,0xBC},  //8.476 ~ 1ms
							    {0xEd,0xA4},{0xF1,0x8C},{0xF5,0x74},{0xF9,0x5C},{0xFC,0x18}};

//步进电机正转的励磁序列
const uchar FFW[] = {0x1F,0x3F,0x2F,0x6F,0x4F,0xCF,0x8F,0x9F}; //DCBAXXXX
const uchar FFW2[] = {0xF1,0xF3,0xF2,0xF6,0xF4,0xFC,0xF8,0xF9}; //DCBAXXXX

//步进电机反转的励磁序列
const uchar REV[] = {0x9F,0x8F,0xCF,0x4F,0x6F,0x2F,0x3F,0x1F}; //DCBAXXXX
const uchar REV2[] = {0xF9,0xF8,0xFC,0xF4,0xF6,0xF2,0xF3,0xF1}; //DCBAXXXX

//枚举变量--正反转标志
typedef enum
	{FwdRun, RevRun} RunFlag;								
RunFlag flag1 = FwdRun;	//默认正转

static number = 0;
static number_x = 0;
static number_y = 0;

//-----------------------------------------------------------------
// 定义ds1302使用的IO口
//-----------------------------------------------------------------
sbit RST=P2^5;  //复位信号端口
sbit SCLK=P2^6; //时钟信号端口
sbit DSIO=P2^7;	//数据信号端口

//---DS1302写入和读取时分秒的地址命令---//
//---秒分时日月周年 最低位读写位;-------//
uchar code READ_RTC_ADDR[7] = {0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d}; 
uchar code WRITE_RTC_ADDR[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c};

//LCD要显示的时间所在的地址
uchar code DT_lcdplace[] = {0x06,0x03,0x00};

//---存储顺序是秒分时日月周年,存储格式是用BCD码---//
uchar TIME[] = {0, 0, 0};

uchar data_h,data_l;

uchar add_flag,drop_flag;

char count=0;		//计数，count=20表示1s

uchar Alarm_time[] = {12,30,00};//闹钟时间

uchar temp1,temp2;

uint ddd;

//-----------------------------------------------------------------
// 延时程序
//-----------------------------------------------------------------
void delay(uchar ms)
{
uchar i;
while(ms--)
{
	for(i=0; i<250; i++)
	{			
		_nop_();
		_nop_();
		_nop_();			
		_nop_();			
	}
}
}
void delay_temp(uchar N)
{
     while(--N);
}

//延时2us
void Delay_2us(void)
{
	_nop_();
	_nop_();
}

void Delay1ms(unsigned int count)
{
	unsigned int i,j;
	for(i=0;i<count;i++)
	for(j=0;j<120;j++);
}

/******************************************************
** 函数名：send
** 描述  ：串口上传pc函数
** 输入  ：要上传的数据
** 输出  ：无
******************************************************/
void send(unsigned char dat)
{
	SBUF=dat;                       	//发送数据
	while(TI==0);                    //检查发送完成中断标志，发送完成为1，未完成为0
	TI=0;                            //复位发送标志位
}	

/*******************************************************************************
* 函 数 名         : Ds1302Write
* 函数功能		   : 向DS1302命令（地址+数据）
* 输    入         : addr,dat
* 输    出         : 无
*******************************************************************************/
void Ds1302Write(uchar addr, uchar dat)
{
	uchar n;
	RST = 0;
	_nop_();

	SCLK = 0;//先将SCLK置低电平。
	_nop_();
	RST = 1; //然后将RST(CE)置高电平。
	_nop_();

	for (n=0; n<8; n++)//开始传送八位地址命令
	{
		DSIO = addr & 0x01;//数据从低位开始传送
		addr >>= 1;
		SCLK = 1;//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;
		_nop_();
	}
	for (n=0; n<8; n++)//写入8位数据
	{
		DSIO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;
		_nop_();	
	}	
		 
	RST = 0;//传送数据结束
	_nop_();
}

/*******************************************************************************
* 函 数 名         : Ds1302Read
* 函数功能		   : 读取一个地址的数据
* 输    入         : addr
* 输    出         : dat
*******************************************************************************/
uchar Ds1302Read(uchar addr)
{
	uchar n,dat,dat1;
	RST = 0;
	_nop_();

	SCLK = 0;//先将SCLK置低电平。
	_nop_();
	RST = 1;//然后将RST(CE)置高电平。
	_nop_();

	for(n=0; n<8; n++)//开始传送八位地址命令
	{
		DSIO = addr & 0x01;//数据从低位开始传送
		addr >>= 1;
		SCLK = 1;//数据在上升沿时，DS1302读取数据
		_nop_();
		SCLK = 0;//DS1302下降沿时，放置数据
		_nop_();
	}
	_nop_();
	for(n=0; n<8; n++)//读取8位数据
	{
		dat1 = DSIO;//从最低位开始接收
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;//DS1302下降沿时，放置数据
		_nop_();
	}

	RST = 0;
	_nop_();	//以下为DS1302复位的稳定时间,必须的。
	SCLK = 1;
	_nop_();
	DSIO = 0;
	_nop_();
	DSIO = 1;
	_nop_();
	return dat;	
}

/*******************************************************************************
* 函 数 名         : Ds1302Init
* 函数功能		   : 初始化DS1302.
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void Ds1302Init()
{
	uchar n;
	EA=0;
	Ds1302Write(0x8E,0X00);		 //禁止写保护，就是关闭写保护功能
	for (n=0; n<7; n++)//写入7个字节的时钟信号：分秒时日月周年
	{
		Ds1302Write(WRITE_RTC_ADDR[n],TIME[n]);	
	}
	Ds1302Write(0x8E,0x80);		 //打开写保护功能
	EA=1;
}

/*******************************************************************************
* 函 数 名         : Ds1302ReadTime
* 函数功能		   : 读取时钟信息
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void Ds1302ReadTime()
{
	uchar n;
	for (n=0; n<3; n++)//读取7个字节的时钟信号：分秒时日月周年
	{
		TIME[n] = Ds1302Read(READ_RTC_ADDR[n]);
	}		
}

void delay_1()
{
    delay(1);
}


// 1602命令函数
void enable(uchar del)
{
        P1 = del;
        rs = 0;
        rw = 0;
        ep = 0;
        delay_1();
        ep = 1;
        delay_1();
}


void write(uchar del)
{
        P1 =del;
        rs = 1;
        rw = 0;
        ep = 0;
        delay_1();
        ep = 1;
        delay_1();
}

// 1602初始化，请参考1602的资料
void L1602_init(void)
{
        enable(0x01);
        enable(0x38);//	设置16*2显示，5*7点阵，8位数据接口
        enable(0x0c);//开显示
        enable(0x06);//读写一字节后地址指针加1
        enable(0xd0);
}


//改变液晶中某位的值，如果要让第一行，第五个字符显示"b" ，调用该函数如下 L1602_char(1,5,'b')
void L1602_char(uchar hang,uchar lie,char sign)
{
        uchar a;
        if(hang == 1) a = 0x80;
        if(hang == 2) a = 0xc0;
        a = a + lie - 1;
        enable(a);
        write(sign);
}

//改变液晶中某位的值，如果要让第一行，第五个字符开始显示"ab cd ef" ，调用该函数如下L1602_string(1,5,"ab cd ef;")
void L1602_string(uchar hang,uchar lie,uchar *p)
{
        uchar a;
        if(hang == 1) a = 0x80;
        if(hang == 2) a = 0xc0;
        a = a + lie - 1;
        enable(a);
        while(1)
        {
                if(*p == '\0') break;
                write(*p);
                p++;
        }
}

/*******************************************************************************
* 函 数 名         : Init_Ds18b20
* 函数功能		  	 : 初始化
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
bit Init_Ds18b20()
{
        bit Status_Ds18b20;
        DQ=1;
        delay_temp(5);
        DQ=0;
        delay_temp(200);
        delay_temp(200);
        DQ=1;
        delay_temp(50);
        Status_Ds18b20=DQ;
        delay_temp(25);
        return Status_Ds18b20;
}

uchar Read_Ds18b20()
{
        uchar i=0,dat=0;
        for(i=0;i<8;i++)
        {
                DQ=0;
                dat>>=1;
                DQ=1;
                if(DQ)
                dat|=0x80;
                delay_temp(25);
        }
        return dat;
}

void Witie_Ds18b20(uchar dat)
{
        uchar i=0;
        for(i=0;i<8;i++)
        {
                DQ=0;
                DQ=dat&0x01;
                delay_temp(25);
                DQ=1;
                dat>>=1;
        }
        delay_temp(25);
}
void DHT11_delay_us(unsigned char n)
{
    while(--n);
}

void DHT11_delay_ms(unsigned int z)
{
   unsigned int i,j;
   for(i=z;i>0;i--)
      for(j=110;j>0;j--);
}

void DHT11_start()
{
   Data=1;
   DHT11_delay_us(2);
   Data=0;
   DHT11_delay_ms(30);   //延时18ms以上
   Data=1;
   DHT11_delay_us(30);
}

unsigned char DHT11_rec_byte()      //接收一个字节
{
   unsigned char i,dat=0;
  for(i=0;i<8;i++)    //从高到低依次接收8位数据
   {
      while(!Data);   ////等待50us低电平过去
      DHT11_delay_us(8);     //延时60us，如果还为高则数据为1，否则为0
      dat<<=1;           //移位使正确接收8位数据，数据为0时直接移位
      if(Data==1)    //数据为1时，使dat加1来接收数据1
         dat+=1;
      while(Data);  //等待数据线拉低
    }
    return dat;
}

void DHT11_receive()      //接收40位的数据
{
    unsigned char R_H,R_L,T_H,T_L,RH,RL,TH,TL,revise;
    DHT11_start();
    if(Data==0)
    {
        while(Data==0);   //等待拉高
        DHT11_delay_us(40);  //拉高后延时80us
        R_H=DHT11_rec_byte();    //接收湿度高八位
        R_L=DHT11_rec_byte();    //接收湿度低八位
        T_H=DHT11_rec_byte();    //接收温度高八位
        T_L=DHT11_rec_byte();    //接收温度低八位
        revise=DHT11_rec_byte(); //接收校正位

        DHT11_delay_us(25);    //结束

        if((R_H+R_L+T_H+T_L)==revise)      //校正
        {
            RH=R_H;
            RL=R_L;
            TH=T_H;
            TL=T_L;
        }
        humi_value = RH;
    }
		
		if(humi_value>60)//如果湿度>60%，则婴儿可能尿床，发出警报
		{
			BUZZER =0;
			LED3 =0;
			flag3=1;
			if(send_flag3++==1)
			{
				send(TIME[2]/16+48);//小时
				send(TIME[2]%16+48);
				send(58);//:
				send(TIME[1]/16+48);//分钟
				send(TIME[1]%16+48);
				send(58);//:
				send(TIME[0]/16+48);//秒
				send(TIME[0]%16+48);
				send(45);//-
				send(119);//w
				send(101);//e
				send(116);//t
				send(32);//空格
			}
		}
		else
		{
			BUZZER =1;
			LED3 =1;
			flag3=0;
			send_flag3=0;
		}
}
/*****************************************
函数简介：获取ADC0832数据
函数名称：ADC_read_data(bit channel)
参数说明：ch为入口参数，ch=0选择通道0，ch=1选择通道1
函数返回：返回读取到的二进制ADC数据，格式为unsigned char
		  当返回一直0时，转换数据有误
******************************************/
uchar ADC_read_data(bit channel)
{
	uchar i,dat0=0,dat1=0;
  //------第1次下降沿之前di置高，启动信号---------

	cs=0;			//片选信号置低，启动AD转换芯片
	clk=0;			//时钟置低平
	
	dio=1;  		//开始信号为高电平
	Delay_2us();
	clk=1;			//产生一个正脉冲,在时钟上升沿，输入开始信号（DI=1）
	Delay_2us();
  clk=0;			//第1个时钟下降沿
	dio=1;
	
	Delay_2us();
	clk=1;		    // 第2个下降沿输入DI=1，表示双通道单极性输入
	Delay_2us();	 
  //------在第2个下降沿，模拟信号输入模式选择（1：单模信号，0：双模差分信号）---------		
	clk=0;	
	dio=channel;         // 第3个下降沿,设置DI，选择通道;
	Delay_2us();
	clk=1;
	Delay_2us();

   //------在第3个下降沿，模拟信号输入通道选择（1：通道CH1，0：通道CH0）------------	
	
	clk=0;
	dio=1;          //第四个下降沿之前，DI置高，准备接收数 
	Delay_2us();	
	for(i=0;i<8;i++)                 //第4~11共8个下降沿读数据（MSB->LSB）
	{
		clk=1;
		Delay_2us();
		clk=0;
		Delay_2us();
		dat0=dat0<<1|dio;
	}
	for(i=0;i<8;i++)                 //第11~18共8个下降沿读数据（LSB->MSB）
	{
		dat1=dat1|((uchar)(dio)<<i);
		clk=1;
		Delay_2us();
		clk=0;
		Delay_2us();
	} 
	cs=1;				  
	return (dat0==dat1)?dat0:0;	    //判断dat0与dat1是否相等
}


//-----------------------------------------------------------------
// 函数功能: LCD显示日期初始化
// 入口参数: 无
// 返回参数: 无
// 全局变量: 无
// 调用模块:   
//----------------------------------------------------------------- 
void date_init(void)
{
	L1602_string(1,1,"00:00:00");
}

/*******************************************************************************
* 函 数 名         : display_temp
* 函数功能		  	 : 显示温度
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_temp()
{
	uint temp;
	float tem;
	Init_Ds18b20();              //复位
	Witie_Ds18b20(0xcc);         //写跳过ROM命令
	Witie_Ds18b20(0x44);         //开启温度转换
	Init_Ds18b20();      
	Witie_Ds18b20(0xcc);
	Witie_Ds18b20(0xbe);         //读暂存器
	TMPL = Read_Ds18b20();
	TMPH = Read_Ds18b20();

	temp = TMPH;
	temp <<= 8;      
	temp = temp | TMPL;

	if(TMPH>8)
	{
					temp=~temp+1;
					FLAG=1;
	}
	else FLAG=0;         
	tem=temp*0.0625;
	temp=tem*100;
	

	if((temp/10000)==0&&(temp/1000%10)==0) //当高位为0时不显示0
					L1602_char(1,11,' ');
	else
					L1602_char(1,11,temp/1000%10 + 48);
	L1602_char(1,12,temp/100%10 + 48);       
	L1602_char(1,13,0x2e);         
	L1602_char(1,14,temp/10%10 + 48);
	L1602_char(1,15,temp%10 + 48);         
	L1602_char(1,16,0xdf);                  //温度符号C前的圈
	L1602_char(1,17,0x43);                  //温度符号C
	if(FLAG==1)
	L1602_char(1,10,0x2d); //输出-号
	else
	L1602_char(1,10,0x2b); //输出+号

	ddd++;
	if(ddd>5)//因为传感器一开始读到的温度数值是默认的85℃，所以延迟一段时间，等数值准确之后再进行判断
	{
		if(temp/100 <15)//如果温度低于15℃，光电报警
		{
			BUZZER =0;
			LED2 =0;
			flag1=1;
			if(send_flag1++==1)
			{
			  send(TIME[2]/16+48);//小时
				send(TIME[2]%16+48);
				send(58);//:
				send(TIME[1]/16+48);//分钟
				send(TIME[1]%16+48);
				send(58);//:
				send(TIME[0]/16+48);//秒
				send(TIME[0]%16+48);
				send(45);//-
				send(99);//c
				send(111);//o
				send(108);//l
				send(100);//d
				send(32);//空格
			}
		}
		else if(temp/100 >40)//如果温度高于40℃，光电报警
		{	
			BUZZER =0;
			LED2 =0;
			flag2=1;
			if(send_flag2++==1)
			{
				send(TIME[2]/16+48);//小时
				send(TIME[2]%16+48);
				send(58);//:
				send(TIME[1]/16+48);//分钟
				send(TIME[1]%16+48);
				send(58);//:
				send(TIME[0]/16+48);//秒
				send(TIME[0]%16+48);
				send(45);//-
				send(104);//h
				send(111);//o
				send(116);//t
				send(32);//空格
			}
		}
		else
		{
			BUZZER =1;
			LED2 =1;
			flag1=0;
			flag2=0;
			send_flag1=0;
			send_flag2=0;
		}
	}
}

/*******************************************************************************
* 函 数 名         : display_humidity
* 函数功能		  	 : 显示湿度
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_humidity()
{
	uint temp;
	temp = humi_value;
	
	L1602_char(2,1,72);                  	 //H
	L1602_char(2,2,117);                   //u
	L1602_char(2,3,109);                   //m
	L1602_char(2,4,58);                    //:       
	L1602_char(2,5,temp/10%10 + 48);			 //湿度的十位数
	L1602_char(2,6,temp%10 + 48);          //湿度的个位数
	L1602_char(2,7,37);                 	 //湿度的%号
}

/*******************************************************************************
* 函 数 名         : get_noise
* 函数功能		  	 : 获取噪音分贝
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void get_noise()
{
	uint temp;
	noise_value = ADC_read_data(0);;
	temp = noise_value*100/255;			//这里进行取整显示,将采集到的电压（0~255）转换到0~100	

	if(temp >60)//如果噪音分贝>60，则表示宝宝哭闹，发出警报
	{
		BUZZER =0;
		LED1 =0;
		flag4=1;
		if(send_flag4++==1)
		{
			send(TIME[2]/16+48);//小时
			send(TIME[2]%16+48);
			send(58);//:
			send(TIME[1]/16+48);//分钟
			send(TIME[1]%16+48);
			send(58);//:
			send(TIME[0]/16+48);//秒
			send(TIME[0]%16+48);
			send(45);//-
			send(99);//c
			send(114);//r
			send(121);//y
			send(32);//空格
		}
	}
	else
	{
		BUZZER =1;
		LED1 =1;
		flag4=0;
		send_flag4=0;
	}
}

/*******************************************************************************
* 函 数 名         : display_noise
* 函数功能		  	 : 显示噪音分贝
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_noise()
{
	uint temp;
	temp = noise_value;
	temp = temp*100/255;			//这里进行取整显示,将采集到的电压（0~255）转换到0~100	
	
	L1602_char(2,8,32);                  	 //空白
	L1602_char(2,9,32);                   
	L1602_char(2,10,32);                   
	L1602_char(2,11,32);                      
	L1602_char(2,12,32);                    
	L1602_char(2,13,temp/10%10 + 48);			 //噪音的十位数
	L1602_char(2,14,temp%10 + 48);         //噪音的个位数
	L1602_char(2,15,100);                  //噪音分贝的d
	L1602_char(2,16,98);                 	 //噪音分贝的b

}

/*******************************************************************************
* 函 数 名         :display_time
* 函数功能		  	 :显示时间
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_time()
{
	uchar i;		
	uchar j;
	j=7;
	for(i=0; i<3; i++)
	{
		data_h=TIME[i]/16;
		data_l=TIME[i]%16;
		
		L1602_char(1,j,data_h+48);
		L1602_char(1,j+1,data_l+48);
		j-=3;	
	}
	L1602_char(1,3,58);//":"
	L1602_char(1,6,58);//":"
}

void PlaySRKL()
{
 	unsigned int i =0,j,k;
	while(SONG_LONG[i]!=0||SONG_TONE[i]!=0)
	{
	 	for(j=0;j<SONG_LONG[i]*20;j++)
		{
		 	BUZZER = ~BUZZER;
			for(k=0;k<SONG_TONE[i]/3;k++);
		}
		Delay1ms(10);
		i++;
	}
}

void di(void)								  //蜂鸣器报警
{
	BUZZER=0;								  //低电平有效，蜂鸣器开始蜂鸣
	delay(100);							  //延时100毫秒，响0.1秒
	BUZZER=1;								  //蜂鸣器停止鸣叫
}

/* 按键扫描 */
unsigned char key_scan(void)
{
	uchar keyval=0;
	if(KEY1 == 0)			//如果按键1按下
	{
		delay(10);	//延时10ms,去除按键抖动
		if(KEY1 == 0)		//再判断一次按键按下
		{	
			return 1; 		//输出键值1
		}	
	}
	if(KEY2 == 0)			//如果按键2按下
	{
		delay(10);	//延时10ms,去除按键抖动
		if(KEY2 == 0)		//再判断一次按键按下
		{		
			return 2; 		//输出键值1
		}	
	}
	if(KEY3 == 0)			//如果按键3按下
	{
		delay(10);	//延时10ms,去除按键抖动
		if(KEY3 == 0)		//再判断一次按键按下
		{		
			return 3; 		//输出键值1
		}	
	}
	
	return 0;				//如果没有按键按下，则输出0
}

void KEY_Control(void)
{
	  if(key_scan() == 1)	
		{
			di();
			flag5=!flag5;
		}
		if(key_scan() == 2)	
		{
			PlaySRKL();
		}
		if(key_scan() == 3)	
		{
			di();
			LIGHT=!LIGHT;
		}
}
	

void main(void)
{ 
	uchar i;
	static step1 = 0;
	
	TMOD|=0x20;               //TMOD=0010 0000,设置定时器T1工作于方式二
	SCON=0x60;                //SCON=0100 0000,设置串口的工作方式为方式1
	PCON=0x00;                //PCON=0000 0000,晶振为11.0592
	TH1=0xf3;                 //设置定时器T1的初值,波特率为2400 
	TL1=0xf3;                 //定时器T1自动填充的值,波特率为2400 
	EA = 1;										//打开总中断	
	TR1=1;                    //启动定时器T1 
	
	L1602_init();
	date_init();
	Ds1302ReadTime();
	TIME[0]&=0X7F;
	Ds1302Init();
	LIGHT=1;//默认婴儿床关灯（睡眠模式）
	while(1)
	{	
		Ds1302ReadTime();			//DS1302读取当前北京时间
		display_time();   		//显示时间
		display_temp();   		//显示温度
		DHT11_receive();			//读取当前湿度
		get_noise();					//读取当前噪音分贝
		if(flag1==0&&flag2==0&&flag3==0&&flag4==0)
		{						
			display_humidity();   //显示湿度
			display_noise();   		//显示噪音分贝
		}
		else
		{
			if(flag1==1)
			{
				L1602_string(2,1,"Temp is too low        ");
			}
			else if(flag2==1)
			{
				L1602_string(2,1,"Temp is too high           ");
			}
			else if(flag3==1)
			{
				L1602_string(2,1,"bed-wetting         ");
			}
			else if(flag4==1)
			{
				L1602_string(2,1,"baby is cryying       ");
			}
			
		}
		if(key_scan() !=0)		//如果按键按下
		{			
		 	KEY_Control();
		}	
		
		if(flag5==1)//开始摇床
		{
			for(i=0;i<82;i++)
			{
				MotorDriver = FFW[step1++];		
				if(step1 == 8) step1 = 0; 
				Delay1ms(1);
			}
			step1 = 0; 
			number_x++;
			number =number_x + number_y*4; 
		}
		
	}			
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 



