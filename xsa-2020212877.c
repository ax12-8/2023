//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include <reg51.h>
#include <intrins.h>

//-----------------------------------------------------------------
// �������ͺ궨��
//-----------------------------------------------------------------
#define uchar unsigned char
#define uint  unsigned int

//-----------------------------------------------------------------
// ����LCDʹ�õ�IO��
//-----------------------------------------------------------------
sbit rs=P2^2;  //�Ĵ���ѡ��˿�
sbit rw=P2^1;  //��дѡ��˿�
sbit ep=P2^0;  //ʹ���źŶ˿�

//-----------------------------------------------------------------
// ���尴���ͷ�����ʹ�õ�IO��
//-----------------------------------------------------------------
sbit KEY1=P0^7;  			//�ֶ������Ȱ�������
sbit KEY2=P0^6;  			//�ֶ���ˮ��������
sbit KEY3=P0^5;  			//�ֶ����Ȱ�������
sbit LED1=P0^4;				//�¶ȹ���led��������
sbit LED2=P2^5;				//�¶ȹ���led��������
sbit keyset=P0^3;  			//�ֶ�Ͷι��������
sbit keyadd=P0^2;  			//�ֶ�Ͷι��������
sbit keysub=P0^1;  			//�ֶ�Ͷι��������
sbit BUZZER=P2^3;  		//����������
sbit HOT=P2^4;  			//��������
sbit ADD_Water=P3^0;	//��ˮ����
sbit OPEN_LED=P0^0;		//�������
sbit COLD = P3^2;			//�����ŷ�����

//-----------------------------------------------------------------
// ����DHT11���ݿ�
//-----------------------------------------------------------------
sbit Data = P3^3; //��ʪ��IO�� 
signed char  humi_value;//ʪ��
signed char  temp_value;//�¶�

//-----------------------------------------------------------------
// ����ADC0832���ݿ�
//-----------------------------------------------------------------
sbit cs = P3^4;//Ƭѡʹ�ܣ��͵�ƽ��Ч
sbit clk = P3^5;//оƬʱ������
sbit dio = P3^7;//��������DI�����DO

uchar add_flag,sun_flag,wind_flag,hot_flag;

uchar Warning_temp[] = {15,35,30,80,50};//�ٽ�ֵ��ʼ������,�¶����ޣ��¶����ޣ�����ʪ�����ޣ�����ǿ�����ޣ�������̼Ũ������

uint guangzhao =0;//������ձ���
uint co2 =0;//����CO2����
//-----------------------------------------------------------------
// ��ʱ����
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

//��ʱ2us
void Delay_2us(void)
{
	_nop_();
	_nop_();
}

// 1602��ʱ����
void delay_1()
{
    delay(1);
}


// 1602�����
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

// 1602��ʼ������ο�1602������
void L1602_init(void)
{
        enable(0x01);
        enable(0x38);//	����16*2��ʾ��5*7����8λ���ݽӿ�
        enable(0x0c);//����ʾ
        enable(0x06);//��дһ�ֽں��ַָ���1
        enable(0xd0);
}


//�ı�Һ����ĳλ��ֵ�����Ҫ�õ�һ�У�������ַ���ʾ"b" �����øú������� L1602_char(1,5,'b')
void L1602_char(uchar hang,uchar lie,char sign)
{
        uchar a;
        if(hang == 1) a = 0x80;
        if(hang == 2) a = 0xc0;
        a = a + lie - 1;
        enable(a);
        write(sign);
}

//�ı�Һ����ĳλ��ֵ�����Ҫ�õ�һ�У�������ַ���ʼ��ʾ"ab cd ef" �����øú�������L1602_string(1,5,"ab cd ef;")
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
������飺��ȡADC0832����
�������ƣ�ADC_read_data(bit channel)
����˵����chΪ��ڲ�����ch=0ѡ��ͨ��0��ch=1ѡ��ͨ��1
�������أ����ض�ȡ���Ķ�����ADC���ݣ���ʽΪunsigned char
		  ������һֱ0ʱ��ת����������
******************************************/
uchar ADC_read_data(bit channel)
{
	uchar i,dat0=0,dat1=0;
  //------��1���½���֮ǰdi�øߣ������ź�---------

	cs=0;			//Ƭѡ�ź��õͣ�����ADת��оƬ
	clk=0;			//ʱ���õ�ƽ
	
	dio=1;  		//��ʼ�ź�Ϊ�ߵ�ƽ
	Delay_2us();
	clk=1;			//����һ��������,��ʱ�������أ����뿪ʼ�źţ�DI=1��
	Delay_2us();
  clk=0;			//��1��ʱ���½���
	dio=1;
	
	Delay_2us();
	clk=1;		    // ��2���½�������DI=1����ʾ˫ͨ������������
	Delay_2us();	 
  //------�ڵ�2���½��أ�ģ���ź�����ģʽѡ��1����ģ�źţ�0��˫ģ����źţ�---------		
	clk=0;	
	dio=channel;         // ��3���½���,����DI��ѡ��ͨ��;
	Delay_2us();
	clk=1;
	Delay_2us();

   //------�ڵ�3���½��أ�ģ���ź�����ͨ��ѡ��1��ͨ��CH1��0��ͨ��CH0��------------	
	
	clk=0;
	dio=1;          //���ĸ��½���֮ǰ��DI�øߣ�׼�������� 
	Delay_2us();	
	for(i=0;i<8;i++)                 //��4~11��8���½��ض����ݣ�MSB->LSB��
	{
		clk=1;
		Delay_2us();
		clk=0;
		Delay_2us();
		dat0=dat0<<1|dio;
	}
	for(i=0;i<8;i++)                 //��11~18��8���½��ض����ݣ�LSB->MSB��
	{
		dat1=dat1|((uchar)(dio)<<i);
		clk=1;
		Delay_2us();
		clk=0;
		Delay_2us();
	} 
	cs=1;				  
	return (dat0==dat1)?dat0:0;	    //�ж�dat0��dat1�Ƿ����
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
   DHT11_delay_ms(30);   //��ʱ18ms����
   Data=1;
   DHT11_delay_us(30);
}

unsigned char DHT11_rec_byte()      //����һ���ֽ�
{
   unsigned char i,dat=0;
  for(i=0;i<8;i++)    //�Ӹߵ������ν���8λ����
   {
      while(!Data);   ////�ȴ�50us�͵�ƽ��ȥ
      DHT11_delay_us(8);     //��ʱ60us�������Ϊ��������Ϊ1������Ϊ0
      dat<<=1;           //��λʹ��ȷ����8λ���ݣ�����Ϊ0ʱֱ����λ
      if(Data==1)    //����Ϊ1ʱ��ʹdat��1����������1
         dat+=1;
      while(Data);  //�ȴ�����������
    }
    return dat;
}

void DHT11_receive()      //����40λ������
{
    unsigned char R_H,R_L,T_H,T_L,RH,RL,TH,TL,revise;
    DHT11_start();
    if(Data==0)
    {
        while(Data==0);   //�ȴ�����
        DHT11_delay_us(40);  //���ߺ���ʱ80us
        R_H=DHT11_rec_byte();    //����ʪ�ȸ߰�λ
        R_L=DHT11_rec_byte();    //����ʪ�ȵͰ�λ
        T_H=DHT11_rec_byte();    //�����¶ȸ߰�λ
        T_L=DHT11_rec_byte();    //�����¶ȵͰ�λ
        revise=DHT11_rec_byte(); //����У��λ

        DHT11_delay_us(25);    //����

        if((R_H+R_L+T_H+T_L)==revise)      //У��
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
* �� �� ��         : display_ws
* ��������		  	 : ��ʾ��ʪ��
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
void display_ws()
{
	DHT11_receive();//��ȡ��ʪ��
	
	/*��ʾ�¶�*/
	L1602_char(1,1,84);//T
	L1602_char(1,2,101);//e
	L1602_char(1,3,109);//m
	L1602_char(1,4,58);//��
	L1602_char(1,5,temp_value/10%10 + 48);//��ʾʮλ
	L1602_char(1,6,temp_value%10 + 48);//��ʾ��λ
  L1602_char(1,7,0xdf);//�¶ȷ��š�
	L1602_char(1,8,0x43);//�¶ȷ���C
		
	if(wind_flag!=1&&hot_flag!=1)//���������£���ִ���Զ��ж�
	{
		if(temp_value <Warning_temp[0])//����¶ȵ���15�棬��籨��������
		{
			HOT =0;
			LED1 =0;
			BUZZER = 0;
		}
		else if(temp_value >Warning_temp[1])//����¶ȸ���35�棬��籨�����ŷ�
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
	
	/*��ʾʪ��*/
	L1602_char(1,10,72);//"H"
	L1602_char(1,11,117);//"u"
	L1602_char(1,12,109);//"m"
	L1602_char(1,13,58);//":"	
	L1602_char(1,14,humi_value%100/10+48);//��ʾʮλ
	L1602_char(1,15,humi_value%10+48);		//��ʾ��λ
	L1602_char(1,16,37);//"%"

	if(add_flag !=1)//���������£���ִ���Զ��ж�
	{
		if(humi_value <Warning_temp[2])//���ʪ��С��30%����ˮ
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
* �� �� ��         : display_CO2()
* ��������		  	 : ��ʾCO2Ũ��
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
void display_CO2()
{
		uchar adc = 0;
		adc = ADC_read_data(0);//��ȡadcֵ,��Χ��1~255������CO2Ũ����100%
		co2 = adc*100/255;			//�������ȡ����ʾ
		
		L1602_char(2,1,67);//"C"
		L1602_char(2,2,79);//"O"
		L1602_char(2,3,50);//"2"
		L1602_char(2,4,58);//":"	
		L1602_char(2,5,co2%100/10+48);//ȡshiduʮλ
		L1602_char(2,6,co2%10+48);		//ȡshidu��λ
		L1602_char(2,7,37);//"%"
	
		if(wind_flag !=1)//���������£���ִ���Զ��ж�
		{
			if(co2 >Warning_temp[3])//���CO2Ũ�ȴ���80%�����ŷ�
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
* �� �� ��         : display_guangzhao
* ��������		  	 : ��ʾ����
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
void display_guangzhao()
{
		uchar adc = 0;
		adc = ADC_read_data(1);//��ȡadcֵ,��Χ��1~255���������ǿ����100%
		guangzhao = adc*100/255;			//�������ȡ����ʾ	
		L1602_char(2,10,83);//"S"
		L1602_char(2,11,117);//"u"
		L1602_char(2,12,110);//"n"
		L1602_char(2,13,58);//":"
		L1602_char(2,14,guangzhao/100+48);//ȡguangzhao��λ
		L1602_char(2,15,guangzhao%100/10+48);//ȡguangzhaoʮλ
		L1602_char(2,16,guangzhao%10+48);		//ȡguangzhao��λ
		
		if(sun_flag!=1)//���������£���ִ���Զ��ж�
		{
			if(guangzhao < Warning_temp[4])//�������С��50%���򿪲����
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

void di(void)								  //����������
{
	BUZZER=0;								  //�͵�ƽ��Ч����������ʼ����
	delay(100);							  //��ʱ100���룬��0.1��
	BUZZER=1;								  //������ֹͣ����
}

/* ����ɨ�� */
unsigned char key_scan(void)
{
	uchar keyval=0;
	if(KEY1 == 0)			//�������1����
	{
		delay(10);	//��ʱ10ms,ȥ����������
		if(KEY1 == 0)		//���ж�һ�ΰ�������
		{	
			return 1; 		//�����ֵ1
		}	
	}
	if(KEY2 == 0)			//�������2����
	{
		delay(10);	//��ʱ10ms,ȥ����������
		if(KEY2 == 0)		//���ж�һ�ΰ�������
		{		
			return 2; 		//�����ֵ2
		}	
	}
	if(KEY3 == 0)			//�������3����
	{
		delay(10);	//��ʱ10ms,ȥ����������
		if(KEY3 == 0)		//���ж�һ�ΰ�������
		{		
			return 3; 		//�����ֵ3
		}	
	}
	if(keyset == 0)			//����������ð���
	{
		delay(10);	//��ʱ10ms,ȥ����������
		if(keyset == 0)		//���ж�һ�ΰ�������
		{
			di();
			while(keyset==0);
			L1602_string(1,1,"Tem:  -   Hum:      ");
			L1602_string(2,1,"CO2:      Sun:      ");
			L1602_char(1,5,Warning_temp[0]/10+48);
			L1602_char(1,6,Warning_temp[0]%10+48);//�¶�����
			L1602_char(1,8,Warning_temp[1]/10+48);
			L1602_char(1,9,Warning_temp[1]%10+48);//�¶�����
			L1602_char(1,15,Warning_temp[2]/10+48);
			L1602_char(1,16,Warning_temp[2]%10+48);//ʪ������
			L1602_char(2,5,Warning_temp[3]/10+48);
			L1602_char(2,6,Warning_temp[3]%10+48);//CO2����
			L1602_char(2,15,Warning_temp[4]/10+48);
			L1602_char(2,16,Warning_temp[4]%10+48);//��������		
			while(keyval!=5)//��������ʱ����򣬰�5���˳�
			{
				if(keyval==0)
				{
					enable(0x80+5);//ѡ����˸				
				}
				if(keyval==1)
				{			
				enable(0x80+8);//ѡ����˸							
				}
				if(keyval==2)
				{			
				enable(0x80+15);//ѡ����˸							
				}
				if(keyval==3)
				{			
				enable(0xC0+5);//ѡ����˸							
				}
				if(keyval==4)
				{			
				enable(0xC0+15);//ѡ����˸							
				}
				enable(0x0f);
				if(keyset==0)//�޸ġ���һ�
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
					L1602_char(1,6,Warning_temp[0]%10+48);//�¶�����
					L1602_char(1,8,Warning_temp[1]/10+48);
					L1602_char(1,9,Warning_temp[1]%10+48);//�¶�����
					L1602_char(1,15,Warning_temp[2]/10+48);
					L1602_char(1,16,Warning_temp[2]%10+48);//ʪ������
					L1602_char(2,5,Warning_temp[3]/10+48);
					L1602_char(2,6,Warning_temp[3]%10+48);//CO2����
					L1602_char(2,15,Warning_temp[4]/10+48);
					L1602_char(2,16,Warning_temp[4]%10+48);//��������		
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
					L1602_char(1,6,Warning_temp[0]%10+48);//�¶�����
					L1602_char(1,8,Warning_temp[1]/10+48);
					L1602_char(1,9,Warning_temp[1]%10+48);//�¶�����
					L1602_char(1,15,Warning_temp[2]/10+48);
					L1602_char(1,16,Warning_temp[2]%10+48);//ʪ������
					L1602_char(2,5,Warning_temp[3]/10+48);
					L1602_char(2,6,Warning_temp[3]%10+48);//CO2����
					L1602_char(2,15,Warning_temp[4]/10+48);
					L1602_char(2,16,Warning_temp[4]%10+48);//��������		
				}

			}
			enable(0x0c);
			L1602_string(1,1,"                     ");
			L1602_string(2,1,"                     ");
			return 4; 		//�����ֵ1
		}	
	}
	return 0;				//���û�а������£������0
}

void KEY_Control(void)
{
	  if(key_scan() == 1)	//�ŷ�
		{
			di();
			COLD=!COLD;				//��ʼ�ŷ�	
			wind_flag =!wind_flag;
		}
		if(key_scan() == 2)	//��ˮ
		{
			add_flag =!add_flag;
			ADD_Water =!ADD_Water;	
			di();
		}
		if(key_scan() == 3)//����
		{
		  hot_flag =!hot_flag;
			HOT =!HOT;	
			di();
		}
}



void main(void)
{ 		
	L1602_init();						//��ʼ��LCD1602	
	
	while(1)
	{	
		display_CO2();				//��ʾCO2Ũ��
		display_guangzhao();	//��ʾ����ǿ��
		display_ws();			  	//��ʾ��ʪ��
		if(key_scan() !=0)		//����������ü�����������ø���ָ���ٽ�ֵ�Ĺ���
		{			
		 	KEY_Control();			//��������
		}	
		
	}			
}


//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 



