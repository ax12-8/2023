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
sbit rs=P2^2;  //寄存器选择端口
sbit rw=P2^1;  //读写选择端口
sbit ep=P2^0;  //使能信号端口

//-----------------------------------------------------------------
// 定义按键和蜂鸣器使用的IO口
//-----------------------------------------------------------------
sbit KEY1=P0^7;  			//手动开风扇按键设置
sbit KEY2=P0^6;  			//手动加水按键设置
sbit KEY3=P0^5;  			//手动加热按键设置
sbit LED1=P0^4;				//温度过低led报警设置
sbit LED2=P2^5;				//温度过高led报警设置
sbit keyset=P0^3;  			//手动投喂按键设置
sbit keyadd=P0^2;  			//手动投喂按键设置
sbit keysub=P0^1;  			//手动投喂按键设置
sbit BUZZER=P2^3;  		//蜂鸣器设置
sbit HOT=P2^4;  			//加热设置
sbit ADD_Water=P3^0;	//加水设置
sbit OPEN_LED=P0^0;		//打光设置
sbit COLD = P3^2;			//风扇排风设置

//-----------------------------------------------------------------
// 定义DHT11数据口
//-----------------------------------------------------------------
sbit Data = P3^3; //温湿度IO口 
signed char  humi_value;//湿度
signed char  temp_value;//温度

//-----------------------------------------------------------------
// 定义ADC0832数据口
//-----------------------------------------------------------------
sbit cs = P3^4;//片选使能，低电平有效
sbit clk = P3^5;//芯片时钟输入
sbit dio = P3^7;//数据输入DI与输出DO

uchar add_flag,sun_flag,wind_flag,hot_flag;

uchar Warning_temp[] = {15,35,30,80,50};//临界值初始化设置,温度下限，温度上限，土壤湿度下限，光照强度下限，二氧化碳浓度上限

uint guangzhao =0;//定义光照变量
uint co2 =0;//定义CO2变量
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

//延时2us
void Delay_2us(void)
{
	_nop_();
	_nop_();
}

// 1602延时函数
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
        temp_value = TH;
    }
}

/*******************************************************************************
* 函 数 名         : display_ws
* 函数功能		  	 : 显示温湿度
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_ws()
{
	DHT11_receive();//读取温湿度
	
	/*显示温度*/
	L1602_char(1,1,84);//T
	L1602_char(1,2,101);//e
	L1602_char(1,3,109);//m
	L1602_char(1,4,58);//：
	L1602_char(1,5,temp_value/10%10 + 48);//显示十位
	L1602_char(1,6,temp_value%10 + 48);//显示个位
  L1602_char(1,7,0xdf);//温度符号°
	L1602_char(1,8,0x43);//温度符号C
		
	if(wind_flag!=1&&hot_flag!=1)//当按键按下，不执行自动判断
	{
		if(temp_value <Warning_temp[0])//如果温度低于15℃，光电报警，加热
		{
			HOT =0;
			LED1 =0;
			BUZZER = 0;
		}
		else if(temp_value >Warning_temp[1])//如果温度高于35℃，光电报警，排风
		{
			COLD =0;
			LED2 =0;
			BUZZER = 0;
		}
		else
		{
			HOT=1;
			COLD=1;
			LED1 =1;
			LED2=1;
			BUZZER = 1;
		}
	}
	
	/*显示湿度*/
	L1602_char(1,10,72);//"H"
	L1602_char(1,11,117);//"u"
	L1602_char(1,12,109);//"m"
	L1602_char(1,13,58);//":"	
	L1602_char(1,14,humi_value%100/10+48);//显示十位
	L1602_char(1,15,humi_value%10+48);		//显示个位
	L1602_char(1,16,37);//"%"

	if(add_flag !=1)//当按键按下，不执行自动判断
	{
		if(humi_value <Warning_temp[2])//如果湿度小于30%，加水
		{
			ADD_Water =0;
			BUZZER = 0;
		}
		else
		{
			ADD_Water=1;
			BUZZER = 1;
		}
	}
	
}

/*******************************************************************************
* 函 数 名         : display_CO2()
* 函数功能		  	 : 显示CO2浓度
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_CO2()
{
		uchar adc = 0;
		adc = ADC_read_data(0);//读取adc值,范围是1~255，假设CO2浓度是100%
		co2 = adc*100/255;			//这里进行取整显示
		
		L1602_char(2,1,67);//"C"
		L1602_char(2,2,79);//"O"
		L1602_char(2,3,50);//"2"
		L1602_char(2,4,58);//":"	
		L1602_char(2,5,co2%100/10+48);//取shidu十位
		L1602_char(2,6,co2%10+48);		//取shidu个位
		L1602_char(2,7,37);//"%"
	
		if(wind_flag !=1)//当按键按下，不执行自动判断
		{
			if(co2 >Warning_temp[3])//如果CO2浓度大于80%，则排风
			{
				COLD =0;
				BUZZER = 0;
			}
			else
			{
				COLD =1;
				BUZZER = 1;
			}
		}
}

/*******************************************************************************
* 函 数 名         : display_guangzhao
* 函数功能		  	 : 显示光照
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void display_guangzhao()
{
		uchar adc = 0;
		adc = ADC_read_data(1);//读取adc值,范围是1~255，假设光照强度是100%
		guangzhao = adc*100/255;			//这里进行取整显示	
		L1602_char(2,10,83);//"S"
		L1602_char(2,11,117);//"u"
		L1602_char(2,12,110);//"n"
		L1602_char(2,13,58);//":"
		L1602_char(2,14,guangzhao/100+48);//取guangzhao百位
		L1602_char(2,15,guangzhao%100/10+48);//取guangzhao十位
		L1602_char(2,16,guangzhao%10+48);		//取guangzhao个位
		
		if(sun_flag!=1)//当按键按下，不执行自动判断
		{
			if(guangzhao < Warning_temp[4])//如果光照小于50%，打开补光灯
			{
				OPEN_LED =0;
				BUZZER = 0;
			}
			else
			{
				OPEN_LED=1;
				BUZZER=1;
			}
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
			return 2; 		//输出键值2
		}	
	}
	if(KEY3 == 0)			//如果按键3按下
	{
		delay(10);	//延时10ms,去除按键抖动
		if(KEY3 == 0)		//再判断一次按键按下
		{		
			return 3; 		//输出键值3
		}	
	}
	if(keyset == 0)			//如果按键设置按下
	{
		delay(10);	//延时10ms,去除按键抖动
		if(keyset == 0)		//再判断一次按键按下
		{
			di();
			while(keyset==0);
			L1602_string(1,1,"Tem:  -   Hum:      ");
			L1602_string(2,1,"CO2:      Sun:      ");
			L1602_char(1,5,Warning_temp[0]/10+48);
			L1602_char(1,6,Warning_temp[0]%10+48);//温度下限
			L1602_char(1,8,Warning_temp[1]/10+48);
			L1602_char(1,9,Warning_temp[1]%10+48);//温度上限
			L1602_char(1,15,Warning_temp[2]/10+48);
			L1602_char(1,16,Warning_temp[2]%10+48);//湿度下限
			L1602_char(2,5,Warning_temp[3]/10+48);
			L1602_char(2,6,Warning_temp[3]%10+48);//CO2上限
			L1602_char(2,15,Warning_temp[4]/10+48);
			L1602_char(2,16,Warning_temp[4]%10+48);//光照下限		
			while(keyval!=5)//进入设置时间程序，按5下退出
			{
				if(keyval==0)
				{
					enable(0x80+5);//选中闪烁				
				}
				if(keyval==1)
				{			
				enable(0x80+8);//选中闪烁							
				}
				if(keyval==2)
				{			
				enable(0x80+15);//选中闪烁							
				}
				if(keyval==3)
				{			
				enable(0xC0+5);//选中闪烁							
				}
				if(keyval==4)
				{			
				enable(0xC0+15);//选中闪烁							
				}
				enable(0x0f);
				if(keyset==0)//修改“下一项”
				{
					delay(10);
					if(keyset==0)
					{
						keyval++;
						di();
						while(keyset==0);
					}
				}
				if(keyadd==0)//++
				{
					delay(10);	
					if(keyadd==0)
					{
						di();
						Warning_temp[keyval]++;		
						while(keyadd==0);
					}
					enable(0x0c);
					L1602_char(1,5,Warning_temp[0]/10+48);
					L1602_char(1,6,Warning_temp[0]%10+48);//温度下限
					L1602_char(1,8,Warning_temp[1]/10+48);
					L1602_char(1,9,Warning_temp[1]%10+48);//温度上限
					L1602_char(1,15,Warning_temp[2]/10+48);
					L1602_char(1,16,Warning_temp[2]%10+48);//湿度下限
					L1602_char(2,5,Warning_temp[3]/10+48);
					L1602_char(2,6,Warning_temp[3]%10+48);//CO2上限
					L1602_char(2,15,Warning_temp[4]/10+48);
					L1602_char(2,16,Warning_temp[4]%10+48);//光照下限		
				}			
				if(keysub==0)//--
				{
					delay(10);	
					if(keysub==0)
					{
						di();
						Warning_temp[keyval]--;
						while(keysub==0);
					}
					enable(0x0c);
					L1602_char(1,5,Warning_temp[0]/10+48);
					L1602_char(1,6,Warning_temp[0]%10+48);//温度下限
					L1602_char(1,8,Warning_temp[1]/10+48);
					L1602_char(1,9,Warning_temp[1]%10+48);//温度上限
					L1602_char(1,15,Warning_temp[2]/10+48);
					L1602_char(1,16,Warning_temp[2]%10+48);//湿度下限
					L1602_char(2,5,Warning_temp[3]/10+48);
					L1602_char(2,6,Warning_temp[3]%10+48);//CO2上限
					L1602_char(2,15,Warning_temp[4]/10+48);
					L1602_char(2,16,Warning_temp[4]%10+48);//光照下限		
				}

			}
			enable(0x0c);
			L1602_string(1,1,"                     ");
			L1602_string(2,1,"                     ");
			return 4; 		//输出键值1
		}	
	}
	return 0;				//如果没有按键按下，则输出0
}

void KEY_Control(void)
{
	  if(key_scan() == 1)	//排风
		{
			di();
			COLD=!COLD;				//开始排风	
			wind_flag =!wind_flag;
		}
		if(key_scan() == 2)	//加水
		{
			add_flag =!add_flag;
			ADD_Water =!ADD_Water;	
			di();
		}
		if(key_scan() == 3)//加热
		{
		  hot_flag =!hot_flag;
			HOT =!HOT;	
			di();
		}
}



void main(void)
{ 		
	L1602_init();						//初始化LCD1602	
	
	while(1)
	{	
		display_CO2();				//显示CO2浓度
		display_guangzhao();	//显示光照强度
		display_ws();			  	//显示温湿度
		if(key_scan() !=0)		//如果按下设置键，则进入设置各项指标临界值的功能
		{			
		 	KEY_Control();			//按键控制
		}	
		
	}			
}


//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 



