
/**
     Copyright (C) 2014 Legent Ltd.
    All rights reserved
  ***************************************************************************************
  *kettle key drivers.
  *
  ***************************************************************************************
  */ 
#include "stm32f2xx.h"
#include "debug.h"
#include "MICOAppDefine.h"
//#include "productdefs.h"
//#include "haprotocol.h"
#include "MicoDriverI2c.h"
#include "MicoDriverGpio.h"
#include "kv.h"
#include "stm32f2xx_gpio.h"
//#include "MICONotificationCenter.h "
#include "mico_wlan.h"
#include "mico.h"

static mico_thread_t KvSsytemTb;//����
static mico_timer_t kv_timer;

static KvState kv_sys_sate;//ϵͳ״̬��
static KvState kv_sys_sate_old;//10ms ֮ǰ��ϵͳ״̬
static bool state_shan250ms_bit;			
static bool state_shan500ms_bit ;		
static bool state_net_bit;			
static bool state_shan1s_bit;	
static bool state_key_keep;
static bool time_10ms_out_flag;
static bool wifi_display;
static u16  wifi_display_timer=0;

static KvTime TIME;
static kv_key KEY;

static u8 STATE_BUZ;//������״̬

static mico_mutex_t KvMutex;//ȫ�ֱ�������
static bool KvMutexInited = FALSE;

#if 0
OsAslQueueT KV_QUE;
#endif


#define mico_log(M, ...) custom_log("MICO", M, ##__VA_ARGS__)
#define mico_log_trace() custom_log_trace("MICO")

#define Set_LED_HIG      GPIO_SetBits(GPIOB,GPIO_Pin_5)// MicoGpioOutputHigh(MICO_GPIO_9)
#define Clr_LED_HIG      GPIO_ResetBits(GPIOB,GPIO_Pin_5)//MicoGpioOutputLow(MICO_GPIO_9)

#define Set_LED_LOW      GPIO_SetBits(GPIOB,GPIO_Pin_8)//MicoGpioOutputHigh(MICO_GPIO_10)
#define Clr_LED_LOW      GPIO_ResetBits(GPIOB,GPIO_Pin_8)//MicoGpioOutputLow(MICO_GPIO_10)


#define Set_LED_NET      GPIO_SetBits(GPIOA,GPIO_Pin_3) //MicoGpioOutputHigh(MICO_GPIO_5)
#define Clr_LED_NET      GPIO_ResetBits(GPIOA,GPIO_Pin_3)// MicoGpioOutputLow(MICO_GPIO_5)


#define Set_LED_LIGHT     GPIO_SetBits(GPIOA,GPIO_Pin_4)// MicoGpioOutputHigh(MICO_GPIO_6)
#define Clr_LED_LIGHT     GPIO_ResetBits(GPIOA,GPIO_Pin_4)// MicoGpioOutputLow(MICO_GPIO_6)


#define Set_LED_POWER     GPIO_SetBits(GPIOA,GPIO_Pin_1)//MicoGpioOutputHigh(MICO_GPIO_11)
#define Clr_LED_POWER     GPIO_ResetBits(GPIOA,GPIO_Pin_1)


#define Set_REST          GPIO_SetBits(GPIOB,GPIO_Pin_14)//MicoGpioOutputHigh(MICO_GPIO_13)
#define Clr_REST          GPIO_ResetBits(GPIOB,GPIO_Pin_14)// MicoGpioOutputLow(MICO_GPIO_13)


#define Get_KEY_POWER     GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)//MicoGpioInputGet(MICO_GPIO_21)//
#define Get_KEY_LOW       GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)//MicoGpioInputGet(MICO_GPIO_16)//С��
#define Get_KEY_HIG       GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)//MicoGpioInputGet(MICO_GPIO_19)//���
#define Get_KEY_LIGHT     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12)//MicoGpioInputGet(MICO_GPIO_16)//����
#define Get_KEY_NET       GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11)//MicoGpioInputGet(MICO_GPIO_21)//MicoGpioInputGet(MICO_GPIO_30)//

//CMD ID ��
#define ID_WIND  0x01
#define ID_LIGHT 0x02
#define ID_BUZZ  0x03
#define ID_READ_CLEAN_TIME 0x04
#define ID_CLR_CLEAN_TIME  0x05
#define ID_READ_SET_HAERT 0x07


//����������
#define wind_off    0x00
#define wind_smal l 0x01
#define wind_middle 0x02
#define wind_large  0x03
#define wind_bao    0x06//����

#define light_on    0x01
#define lingt_off   0x00//������

//ϵͳ״̬�����
#define sys_off 0
#define sys_on  1
#define sys_delay_off 2

mico_i2c_device_t powrboard;

void KvVarProtect(bool ProtectOn)
{
  if(KvMutexInited)
  {
    if(ProtectOn)
    {
      mico_rtos_lock_mutex(&KvMutex);
    }
    else
    {
      mico_rtos_lock_mutex(&KvMutex);
    }
  }
}

/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------ɨ���ź�ǿ����ʾ �ص�����------------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

void  scan_wifi_complete_call_back(ScanResult *wifi_si_result ,mico_Context_t * const context_tmp)
{
  u8 j=0;
  mico_log("the sisi num is %d",wifi_si_result->ApNum);
  if(0!=wifi_si_result->ApNum)
  {
    for(j=0;j<wifi_si_result->ApNum;j++)
    {
      mico_log("the ap name is %s",(wifi_si_result->ApList+j)->ssid);
      //printf("the ap name is %s",(wifi_si_result->ApList+i)->ssid);
      mico_log("the ap signal strength is %d",(wifi_si_result->ApList+j)->ApPower);
      if(0==strcmp(context_tmp->flashContentInRam.micoSystemConfig.ssid,(char *)(wifi_si_result->ApList+j)->ssid))
      {
        if((wifi_si_result->ApList+j)->ApPower<40)                                          //--1
        {
          Clr_LED_HIG;
          Clr_LED_LIGHT;
          Set_LED_NET;             
        }
        if((wifi_si_result->ApList+j)->ApPower>40&&(wifi_si_result->ApList+j)->ApPower<70) //--2
        {
          Clr_LED_HIG;
          Set_LED_LIGHT;
          Set_LED_NET;                  
        }
        if((wifi_si_result->ApList+j)->ApPower>70)
        {
          Set_LED_HIG;
          Set_LED_LIGHT;
          Set_LED_NET;              
        } 
      }   //mico_log(wifi_si_result->ApList->ssid);
    }   
  } 
}


/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------ʱ��������------------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	time_dis(mico_i2c_device_t * slave_device)
{

	 if(wifi_display==1)
     {
        wifi_display_timer++;
		if(wifi_display_timer==500)
		{
		  wifi_display=0;
		  wifi_display_timer=0;
		}

	 }
	 if(wifi_display==0)
	 {
		 time_shan_dis();//��˸����
		 gjys_1f_dis(slave_device);//�ػ�1������ʱ����
		 key_check(slave_device);
		 baoc_dis(slave_device);
		 out_dis(slave_device);
		 fan_time_dis(slave_device);
		 Heart_time_dis(slave_device);
		 proces_out_dis(slave_device);
		
	 }
		
	
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------1������ʱ�ػ�����------------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	gjys_1f_dis(mico_i2c_device_t *slave_device)
{
     SlaveMessge buzz_messge;
  KvVarProtect(TRUE);  
	if(kv_sys_sate.STATE_SYS == 2)//����ʱ�ػ�״̬
	{
		TIME.TIME_YS_10MS++;
		if(TIME.TIME_YS_10MS >= 6000)//��ʱS��ʱ
		{
				//BUZZ.STATE_BUZ = 3;//�ػ����� 
				/*buzz_messge.id=ID_BUZZ
			    buzz_messge.value=buzz_system_off;
				if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
				{
				   mico_log("set the buzze vaue failed");
				}*/
				STATE_BUZ=buzz_system_off;
				kv_sys_sate.STATE_SYS = 0;//�ػ�״̬   
				
		}
	}
	else//������ʱ�ػ�״̬
	{
		TIME.TIME_YS_10MS = 0;
	}
  KvVarProtect(FALSE);
}

/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

0.25���0��5����˸
//---------0.5S��˸ʱ�䴦��
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	time_shan_dis(void)
{
	TIME.TIME_SHAN_10MS++;
	if(TIME.TIME_SHAN_10MS>= 50)
	{
		TIME.TIME_SHAN_10MS = 0;
		if(state_shan500ms_bit)
			state_shan500ms_bit =0;
		else
			state_shan500ms_bit=1;
	}
	TIME.TIME_SHAN2_10MS++;
	if(TIME.TIME_SHAN2_10MS>= 25)
	{
		TIME.TIME_SHAN2_10MS = 0;
		if(state_shan250ms_bit)
			state_shan250ms_bit =0;
		else
			state_shan250ms_bit=1;
	}	
	TIME.TIME_SHAN3_10MS++;
	if(TIME.TIME_SHAN3_10MS>= 100)
	{
		TIME.TIME_SHAN3_10MS = 0;
		if(state_shan1s_bit)
			state_shan1s_bit =0;
		else
			state_shan1s_bit=1;
	}	

}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------����ʱ�䴦��------------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	baoc_dis(mico_i2c_device_t *slave_device)
{
  SlaveMessge buzz;
  KvVarProtect(TRUE);    
  if(kv_sys_sate.STATE_WIND == 6||kv_sys_sate.STATE_WIND==baochaochange)
  {
    TIME.TIME_BAOC_10MS++;
    
    if(TIME.TIME_BAOC_10MS >= 17960 && TIME.TIME_BAOC_10MS <18000)//����ʱ��
    {      
      kv_sys_sate.STATE_WIND =baochaochange;      
    }
    
    if(TIME.TIME_BAOC_10MS >= 18000)//����ʱ��
    {
      TIME.TIME_BAOC_10MS = 0;
      /*
          buzz.value= buzz_one_sing;
          buzz.id=ID_BUZZ;
          if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
          {
            mico_log("set the buzze vaue failed");
          }
         */
      STATE_BUZ=buzz_one_sing;
      kv_sys_sate.STATE_WIND = 3;
    }
  }	

  KvVarProtect(FALSE);
}

void    proces_init_out_dis(mico_i2c_device_t *slave_device)
{
  SlaveMessge s_messge;
  s_messge.id=ID_WIND;
  KvVarProtect(TRUE);

  s_messge.value=kv_sys_sate.STATE_WIND;
  if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
  {
    mico_log("set the wind vaue failed");
  } 
  s_messge.id=ID_LIGHT;
  s_messge.value=kv_sys_sate.STATE_LIGHT;
  if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
  {
    mico_log("set the light vaue failed");
  } 
  if(buzz_silent!=STATE_BUZ)	  
  {
    s_messge.id=ID_BUZZ;
    s_messge.value=STATE_BUZ;
    if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
    {
      mico_log("set the buzze vaue failed");
    } 
    STATE_BUZ=buzz_silent;
  }
  KvVarProtect(FALSE);
}

void  seng_key_messge(KvKeyId kv_key_id,bool short_long)
{
#if 0 //qxf need't
	OsAslMsgT KEY_VALUE_MSG;
  memset(&KEY_VALUE_MSG, 0, sizeof(KEY_VALUE_MSG));
	//KvKeyMessge keymessge;
	//keymessge.key_id=kv_key_id;
	//keymessge.key_short_long=short_long;
	KEY_VALUE_MSG.Arg=kv_key_id;
	KEY_VALUE_MSG.Arg2=short_long;
 // KEY_VALUE_MSG.MsgId = RH_KEY_PRESS_EVENT_MSG;
#if 0  
	if(kNoErr!=OsAslMsgQueueSendMsg(&KV_QUE,&KEY_VALUE_MSG,OS_ASL_WAIT_FOREVER))
	{
        mico_log("send key value mess failed----------failed");

	}
#endif
  //SendMsgToRhTask(&KEY_VALUE_MSG);
#endif
}

/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					�������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	key_check(mico_i2c_device_t *slave_device)
{
    //SlaveMessge slave_messge;
    KvKeyMessge key_messge;

	Set_REST;//�����������
	
  KvVarProtect(TRUE);
  
	if(!state_key_keep)
	{         
//��������
          if(!Get_KEY_HIG)
          {
			KEY.KEY_HIG++;
			if(KEY.KEY_HIG == 300)
            {
				   mico_log("in high wind key\r\n");
				   state_key_keep=1;
 ///////////////////////////////////////////////////////////////////////
                   micoWlanStartScan();
                   wifi_display=1;
 
///////////////////////////////////////////////////////////////////////			
			}	  
          }
          else
          	{
 ///////////////////////////////////////////////////////////////////////
 //�̰�������
				if(KEY.KEY_HIG>15&&KEY.KEY_HIG<100)
                  {
                    state_key_keep=1;
                   /********�ϱ�������ֵ**********************************/
				    seng_key_messge(kv_key_hig,KEY_SHORT);
				  /**************---------------------------******************/
			/*****************************************************�������ݿ���ע��**********************************/
                    if(kv_sys_sate.STATE_SYS ==1)
                        {
								STATE_BUZ=buzz_one_sing;
                                if(kv_sys_sate.STATE_WIND == 6||kv_sys_sate.STATE_WIND == 3)//����
                                        kv_sys_sate.STATE_WIND = 0;
                                else
                                        kv_sys_sate.STATE_WIND = 6;		
                        }
                        else if(kv_sys_sate.STATE_SYS ==2)//��ʱ�ػ��˳���ʱ
                        {
                               
                                STATE_BUZ=buzz_one_sing;
                                kv_sys_sate.STATE_SYS = 1;
                        }
		  /**************----------------------------------------------------------------------------------------------------------------******************/
                  }
///////////////////////////////////////////////////////////////////////			
            	KEY.KEY_HIG=0;//
          }
			
 //С�������                       
           if(!Get_KEY_LOW)
          {
                  KEY.KEY_LOW++;
                  if(KEY.KEY_LOW == 15)
                  {
                     KEY.KEY_LOW= 0;
                     state_key_keep=1;
                    /********�ϱ�������ֵ**********************************/
				    seng_key_messge(kv_key_low,KEY_SHORT);
					/**************---------------------------******************/
			/*****************************************************�������ݿ���ע��**********************************/
          #if 0
                    if(kv_sys_sate.STATE_SYS == 1)
                    {
                            
							STATE_BUZ=buzz_one_sing;
                            if(kv_sys_sate.STATE_WIND ==1)
                                    kv_sys_sate.STATE_WIND = 0;
							else if(kv_sys_sate.STATE_WIND == 2)
								   kv_sys_sate.STATE_WIND = 1;
							else if(kv_sys_sate.STATE_WIND == 3)
								   kv_sys_sate.STATE_WIND = 2;
                           	else 
                                   kv_sys_sate.STATE_WIND = 3;
            
                    }
          #else
                if(kv_sys_sate.STATE_SYS == 1)
                {
                        
                        STATE_BUZ=buzz_one_sing;
                        if(kv_sys_sate.STATE_WIND ==2 || kv_sys_sate.STATE_WIND ==1)
                                kv_sys_sate.STATE_WIND = 0;
                        else 
                               kv_sys_sate.STATE_WIND = 2;
          
                }          
          #endif
                    else if(kv_sys_sate.STATE_SYS == 2)//��ʱ�ػ��˳���ʱ
                    {
                         
                           STATE_BUZ=buzz_one_sing;
                            kv_sys_sate.STATE_SYS = 1;	
                    }
		/**************----------------------------------------------------------------------------------------------------------------******************/
                  }
          }
          else
            KEY.KEY_LOW=0;
//����������
          if(!Get_KEY_LIGHT)
          {
				  KEY.KEY_LIGHT++;
                  if(KEY.KEY_LIGHT == 15)
                  {
                    KEY.KEY_LIGHT= 0;
                    state_key_keep=1;
				    /********�ϱ�������ֵ**********************************/
				    seng_key_messge(kv_key_light,KEY_SHORT);
					/**************---------------------------******************/
		       /*****************************************************�������ݿ���ע��**********************************/
                   // BUZZ.STATE_BUZ = 1;
                   STATE_BUZ=buzz_one_sing;
                    if(kv_sys_sate.STATE_LIGHT)
                            kv_sys_sate.STATE_LIGHT=0;
                    else
                            kv_sys_sate.STATE_LIGHT=1; 
	/**************----------------------------------------------------------------------------------------------------------------******************/
                  }
          }
          else
            KEY.KEY_LIGHT=0;  
//���簴������
          if(!Get_KEY_NET)/////////////////////////////////////////////////////////���簴������
          {
                  KEY.KEY_NET++;
                  if(KEY.KEY_NET==300)
                  {
                    state_key_keep=1;
				    /********�ϱ�������ֵ**********************************/
				    seng_key_messge(kv_key_net,KEY_LONG);
					/**************---------------------------******************/
					/*****************************************************�˴�����������������**********************************/
				    STATE_BUZ=buzz_one_sing;
                   // kv_sys_sate.STATE_NET=0x02;
                    /**************----------------------------------------------------------------------------------------------------------------******************/
                  }
          }
          else
          {
            if((KEY.KEY_NET>15)&&(KEY.KEY_NET<100))
            {
               state_key_keep=1;
			   /********�ϱ�������ֵ**********************************/
			   seng_key_messge(kv_key_net,KEY_SHORT);
			  /****************---------------------------******************/
			  
			  /*****************************************************�˴����������̰�����**********************************/
			  STATE_BUZ=buzz_one_sing;
			  /**************----------------------------------------------------------------------------------------------------------------******************/
            //  kv_sys_sate.STATE_NET=0x01;    
           
			   
            }
            KEY.KEY_NET=0;             
           }
 //���ؼ�����
          if(!Get_KEY_POWER)
          {
                  KEY.KEY_POWER++;
                  if(KEY.KEY_POWER == 15)
                  {
                   // mico_log("come here the power key buuton");
                    /********�ϱ�������ֵ**********************************/
				    seng_key_messge(kv_key_power,KEY_SHORT);
					/**************---------------------------******************/
					KEY.KEY_POWER= 0;
                    state_key_keep=1;
	   /*****************************************************�������ݿ���ע��**********************************/
                    if(kv_sys_sate.STATE_SYS == 1)
                    {
                            if(kv_sys_sate.STATE_WIND == 0)
                            {
                                   // BUZZ.STATE_BUZ = 3;
                                    STATE_BUZ=buzz_system_off;
                                   /* slave_messge.id=ID_BUZZ
									slave_messge.value=buzz_system_off;
									if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
									{
										mico_log("set the buzze vaue failed");
									}*/
                                    kv_sys_sate.STATE_SYS = 0;
                                    kv_sys_sate.STATE_LIGHT = 0;			
                            }
                            else
                            {
                                    //BUZZ.STATE_BUZ = 1;
									STATE_BUZ=buzz_one_sing;
                                   /* slave_messge.id=ID_BUZZ
									slave_messge.value=buzz_one_sing;
									if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
									{
										mico_log("set the buzze vaue failed");
									}*/
                                    kv_sys_sate.STATE_SYS = 2;	
                            }
                    }
                    else if(kv_sys_sate.STATE_SYS == 2)
                    {
                            //BUZZ.STATE_BUZ = 3;
                            STATE_BUZ=buzz_system_off;
                           /* slave_messge.id=ID_BUZZ
							slave_messge.value=buzz_system_off;
							if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
							{
								mico_log("set the buzze vaue failed");
							}*/
                            kv_sys_sate.STATE_SYS = 0;
                            kv_sys_sate.STATE_LIGHT = 0;						
                    }
                    else
                    {
                            //BUZZ.STATE_BUZ = 2;	
                            STATE_BUZ=buzz_system_on;
                           /* slave_messge.id=ID_BUZZ
							slave_messge.value=buzz_system_on;
							if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
							{
								mico_log("set the buzze vaue failed");
							}   */         
                            kv_sys_sate.STATE_SYS = 1;					
                    }	
	/**************----------------------------------------------------------------------------------------------------------------******************/
                  }
          }
          else
            KEY.KEY_POWER=0; 

	}	

  KvVarProtect(FALSE);
  
    if(Get_KEY_HIG&&Get_KEY_LOW&&Get_KEY_LIGHT&&Get_KEY_NET&&Get_KEY_POWER)  
    {
      	 //KEY.KEY_KEEP++; 
      	 state_key_keep=0;
       	KEY.KEY_KEEP=0;
    }
    else
    {
    	KEY.KEY_KEEP++; 
      	//state_key_keep=0;
       //	KEY.KEY_KEEP=0;
    }
	/*if((KEY.KEY_LOW||KEY.KEY_HIG||KEY.KEY_LIGHT||KEY.KEY_NET||KEY.KEY_POWER)==0)  
    {
    	state_key_keep=0;
     	KEY.KEY_KEEP=0; 
     	//KEY.KEY_KEEP++;
    }
    else
    {
    	KEY.KEY_KEEP++;
    	//state_key_keep=0;
     	//KEY.KEY_KEEP=0;
    }*/
    if(KEY.KEY_KEEP>500)//��λ����
      Clr_REST;     
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	led�������			 
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	out_dis(mico_i2c_device_t *slave_device)
{
  KvVarProtect(TRUE);

   if(wifi_display==0)
   {
	if(kv_sys_sate.STATE_SYS == 0)//�ػ�״̬���������
		kv_sys_sate.STATE_WIND=0;
		fan_out_dis(slave_device);
		power_led_dis();
		net_out_dis();
		light_out_dis(slave_device);
   }
   KvVarProtect(FALSE);
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					�������������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

void	net_out_dis(void)
{
/*0:�أ�1:������2:��˸2Hz��3:��˸1Hz��4:��˸0.5Hz*/
  KvVarProtect(TRUE);

	switch(kv_sys_sate.STATE_NET)//
	{
	case 0://��
		Clr_LED_NET;
		break;
	case 1://��
		Set_LED_NET;
		break;
	case 2://2Hz
		if(state_shan250ms_bit)
		{
			Set_LED_NET;
		}
		else
		{
			Clr_LED_NET;
		}
		break;
	case 3://1Hz
		if(state_shan500ms_bit)
		{
			Set_LED_NET;
		}
		else
		{
			Clr_LED_NET;
		}
		break;
	case 4://0.5Hz
		if(state_shan1s_bit)
		{
			Set_LED_NET;
		}
		else
		{
			Clr_LED_NET;
		}
		break;
	default:
		
		break;
	}
	KvVarProtect(FALSE);	  
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					��Դ���������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	power_led_dis()
{
  KvVarProtect(TRUE);

	if(kv_sys_sate.STATE_SYS == 2)//�������ʱ�ػ�״̬���
	{
		if(state_shan500ms_bit)//��˸���
			Set_LED_POWER;
		else
			Clr_LED_POWER;
	}
	else if(kv_sys_sate.STATE_SYS == 1)
		Set_LED_POWER;
	else
		Clr_LED_POWER;
  
  KvVarProtect(FALSE);  
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					�����������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void	fan_out_dis(mico_i2c_device_t *slave_device)
{
    //SlaveMessge wind_messge;
  KvVarProtect(TRUE);    
	switch(kv_sys_sate.STATE_WIND)
	{
	case 1:
		Clr_LED_HIG;
		Set_LED_LOW;
		break;
	case 2:
		//Clr_LED_LOW;
		Clr_LED_HIG;
		Set_LED_LOW;
		break;
	case 3:
		//Set_LED_HIG;
	#if 0	
		Clr_LED_HIG;;
		Set_LED_LOW;
  #else
		Clr_LED_LOW;
		Set_LED_HIG;  
  #endif
		break;
	case 6:
		Clr_LED_LOW;
		Set_LED_HIG;
		break;
	default:
		Clr_LED_LOW;
		Clr_LED_HIG;
		break;
	}
 /*   if(kv_sys_sate_old.STATE_WIND!=kv_sys_sate.STATE_WIND)
    {
      wind_messge.id=ID_WIND;
	  wind_messge.value=kv_sys_sate.STATE_WIND;
	  if(FALSE==Set_Slave_Vaule(slave_device,&slave_messge))
	  {
		mico_log("set the wind vaue failed");
	  } 

	}*/
  KvVarProtect(FALSE);	
}
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					�������������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
void light_out_dis(mico_i2c_device_t *slave_device)
{
  //  SlaveMessge light_messge;
  KvVarProtect(TRUE);  
	if(kv_sys_sate.STATE_LIGHT)
		Set_LED_LIGHT;
	else
		Clr_LED_LIGHT;
  KvVarProtect(FALSE);
	/*if(kv_sys_sate_old.STATE_WIND!=kv_sys_sate.STATE_WIND)
    {
      light_messge.id=ID_WIND;
	  light_messge.value=kv_sys_sate.STATE_WIND;
	  if(FALSE==Set_Slave_Vaule(slave_device,&light_messge))
	  {
		mico_log("set the wind vaue failed");
	  } 

	}*/
}

/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
					���������������״̬ͨ��i2c �������
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

void    proces_out_dis(mico_i2c_device_t *slave_device)
{
  SlaveMessge s_messge;
  KvVarProtect(TRUE);

  if(kv_sys_sate_old.STATE_WIND!=kv_sys_sate.STATE_WIND && kv_sys_sate.STATE_WIND!=baochaochange)     
  {
    s_messge.id=ID_WIND;
    s_messge.value=kv_sys_sate.STATE_WIND;
    if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
    {
      mico_log("set the wind vaue failed");
    } 
  }
  
  if(kv_sys_sate_old.STATE_LIGHT!=kv_sys_sate.STATE_LIGHT)	  
  {
    s_messge.id=ID_LIGHT;
    s_messge.value=kv_sys_sate.STATE_LIGHT;
    if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
    {
      mico_log("set the light vaue failed");
    } 
  }
  
  if(buzz_silent!=STATE_BUZ)	  
  {
    s_messge.id=ID_BUZZ;
    s_messge.value=STATE_BUZ;
    if(FALSE==Set_Slave_Vaule(slave_device,&s_messge))
    {
      mico_log("set the buzze vaue failed");
    } 
    STATE_BUZ=buzz_silent;
  }
     
  kv_sys_sate_old.STATE_SYS=kv_sys_sate.STATE_SYS;
  kv_sys_sate_old.STATE_LIGHT=kv_sys_sate.STATE_LIGHT;
  kv_sys_sate_old.STATE_NET=kv_sys_sate.STATE_NET;
  kv_sys_sate_old.STATE_WIND=kv_sys_sate.STATE_WIND;
  
  KvVarProtect(FALSE);	   
}


static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength)
  {
    if ((*pBuffer1) != *pBuffer2)
    {
      return BufferLength;
    }
    pBuffer1++;
    pBuffer2++;
	BufferLength--;
  }
  return 0;
}


/*
  *Function name: I2c_slave_Inital
  *Descritpion:��ʼ��I2Cģ�� �Լ����豸��ַ
  *input:

  *output:
  
*/ 

void  I2c_slave_Inital(mico_i2c_device_t *slave_deive)
{
  
  slave_deive->address=0xA0>>1;
  slave_deive->address_width=I2C_ADDRESS_WIDTH_7BIT;
  slave_deive->port=MICO_I2C_1;
  //slave_deive->speed_mode=I2C_STANDARD_SPEED_MODE;
  slave_deive->speed_mode=I2C_STANDARD_SPEED_MODE;
  if(kNoErr== MicoI2cInitialize(slave_deive))
  { 
    mico_log("inital the the i2c slave -power board sucess");  
  }
}


/*
  *Function name: I2c_slave_Io_Inital
  *Descritpion: I2c nitialization the i2c pin PB6 PB7
  *input:

  *output:
  
*/ 
void  I2c_slave_Io_Inital(void *arg)
{
  MicoGpioInitialize(MICO_GPIO_1,OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioInitialize(MICO_GPIO_2,OUTPUT_OPEN_DRAIN_NO_PULL );
}

/*
  *Function name: check_the_slave_device_on_line
  *Descritpion: check the i2c slave device is online or not
  *input:

  *output:
  
*/

bool check_slave_device_on_line(mico_i2c_device_t *slave_deive)
{
  return  MicoI2cProbeDevice(slave_deive,1);
}




/*
  *Function name: Set_Slave_Vaule
  *Descritpion: ����pwoer board�ķ��� ���� ����
  *input: SlaveMessge *messge ���豸��Ϣ ����id���Լ�����ֵ 
   ��У�� ���У��ֵ

  *output: ture ��ֵ�ɹ���false ����ʧ��
  
*/
bool  Set_Slave_Vaule(mico_i2c_device_t *slave_device,SlaveMessge *messge)
{
     u8 i=0;
     mico_i2c_message_t message_tx;
	 mico_i2c_message_t message_rx;
	 SlaveMessge messge_tmp;
     message_tx.rx_buffer=NULL;
     message_tx.rx_length=0;
	 MicoI2cBuildTxMessage(&message_tx,messge,4,3);
     message_rx.tx_buffer=NULL;
     message_rx.tx_length=0;
     MicoI2cBuildRxMessage(&message_rx,&messge_tmp,4,3);
	 messge->sum_check=messge->id+messge->value;
	 messge->xor_check=messge->id^messge->value;
	 for(i=0;i<3;i++)//��෢������
	 {
		 if(kNoErr!=MicoI2cTransfer(slave_device,&message_tx,1))
	     {
	        mico_log("tx the slave messge fasule");
			I2C_Cmd ( I2C1, DISABLE);
			mico_thread_msleep(1);
			I2C_Cmd ( I2C1, ENABLE);
			I2c_slave_Inital(slave_device);
		 }
		 HAL_Delay(1);
		 messge_tmp.sum_check=0;
		 if(kNoErr!=MicoI2cTransfer(slave_device,&message_rx,1))
	     {
	        mico_log("recive the slave messge fasule");
			I2C_Cmd ( I2C1, DISABLE);
			mico_thread_msleep(1);
			I2C_Cmd ( I2C1, ENABLE);
			I2c_slave_Inital(slave_device);
		 }
		 if(0==Buffercmp((u8 *)messge,(u8 *)&messge_tmp,4))
		 {
          
			return TRUE;
		 }
		  mico_log("set the value times is %d",i);
	 }
     return FALSE;
}

/*
  *Function name: Read_Slave_Vaule
  *Descritpion: ����pwoer board�ķ��� ���� ����
  *input: SlaveMessge *messge ���豸��Ϣ ����id���Լ�����ֵ 
   ��У�� ���У��ֵ

  *output:�ظ���ȡ�����ݣ�false ����ʧ��
  
*/
u8 Read_Slave_Vaule(mico_i2c_device_t *slave_device,SlaveMessge *messge)
{
     u8 i=0;
     mico_i2c_message_t message_tx;
	 mico_i2c_message_t message_rx;
	 SlaveMessge messge_tmp;
     message_tx.rx_buffer=NULL;
     message_tx.rx_length=0;
	 MicoI2cBuildTxMessage(&message_tx,messge,4,3);
     message_rx.tx_buffer=NULL;
     message_rx.tx_length=0;
     MicoI2cBuildRxMessage(&message_rx,&messge_tmp,4,3);
	 messge->sum_check=messge->id+messge->value;
	 messge->xor_check=messge->id^messge->value;
	 for(i=0;i<3;i++)//��෢������
	 {
		 if(kNoErr!=MicoI2cTransfer(slave_device,&message_tx,1))
	     {
	        mico_log("tx the slave messge fasule");
			I2C_Cmd ( I2C1, DISABLE);
			mico_thread_msleep(1);
			I2C_Cmd ( I2C1, ENABLE);
			I2c_slave_Inital(slave_device);//����i2C ģ��
		 }
		 HAL_Delay(1);
		 messge_tmp.sum_check=0;
		 if(kNoErr!=MicoI2cTransfer(slave_device,&message_rx,1))
	     {
	        mico_log("recive the slave messge fasule");
			I2C_Cmd ( I2C1, DISABLE);
			mico_thread_msleep(1);
			I2C_Cmd ( I2C1, ENABLE);
			I2c_slave_Inital(slave_device);//����i2C ģ��
		 }
		 if((messge_tmp.sum_check==messge_tmp.id+messge_tmp.value)&&(messge_tmp.xor_check==messge_tmp.id^messge_tmp.value))
		 {
		   return messge_tmp.value;
		 }
		 else
		 {
		   mico_log("the rcive value is %d ",messge_tmp.value);
		 }
		 
	 }
     return FALSE;
}



/*
  *Function name: kv_system_inital
  *Descritpion: ��ʼ��ģ��IO �Լ��̻�״̬
  *input:

  *output:
  
*/
void kv_system_inital(KvState *kv_state)
{
  I2c_slave_Io_Inital(NULL);
  /***------------/���ָʾ pin 9-PB5---------------------***/
  GPIO_InitTypeDef Io_init_type;
  Io_init_type.GPIO_Pin=GPIO_Pin_5;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  /***------------С���ָʾ pin 10-PB8---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_8;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  /***------------����ָʾ�� pin 5-PA3---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_3;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&Io_init_type);
  /***------------����ָʾ�� pin 6-PA4---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_4;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&Io_init_type);
  /***------------��Դָʾ��pin 11-PA1---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_1;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&Io_init_type);
  /***------------����������λ��pin 13-PB14---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_14;
  Io_init_type.GPIO_Mode=GPIO_Mode_OUT;
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;
  Io_init_type.GPIO_OType=GPIO_OType_PP;
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  /***------------���簴������pin 21-PA11---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_11;
  Io_init_type.GPIO_Mode=GPIO_Mode_IN;//����
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;//��������
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&Io_init_type);
  /***----------������������pin 20-PA12---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_12;
  Io_init_type.GPIO_Mode=GPIO_Mode_IN;//����
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;//��������
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&Io_init_type);
  /***----------��簴������pin 19-PB11---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_11;
  Io_init_type.GPIO_Mode=GPIO_Mode_IN;//����
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;//��������
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  /***----------С�簴������pin 16-PB1---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_1;
  Io_init_type.GPIO_Mode=GPIO_Mode_IN;//����
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;//��������
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  /***---------��Դ��������pin 30-PB9---------------------***/
  Io_init_type.GPIO_Pin=GPIO_Pin_9;
  Io_init_type.GPIO_Mode=GPIO_Mode_IN;//����
  Io_init_type.GPIO_PuPd=GPIO_PuPd_UP;//��������
  Io_init_type.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOB,&Io_init_type);
  
  /**�̻�ϵͳ״̬��ʼ��**/
  kv_state->STATE_WIND=0;
  kv_state->STATE_SYS=0;
  kv_state->STATE_NET=0;
  kv_state->STATE_LIGHT=0;
  return;
}


/*
  *Function name: kv_timer_handle
  *Descritpion: �̻���ʱ��������
  *input:

  *output:
  
*/

void   kv_timer_handle(void *arg)
{
   //mico_log("the timer is reached-------------------------------------the timer is reacehd");
   //whx 
   mico_stop_timer(&kv_timer);
   
   time_10ms_out_flag=1;//10ms ��ʱ��
}


/*
  *Function name: Kv_Task
  *Descritpion: �̻�����
  *input:

  *output:
  
*/
void Kv_Task(void *arg)
{
  mico_i2c_message_t test_message_tx;
  mico_i2c_message_t test_message_rx;
  mico_i2c_message_t test_message;
  OsAslMsgT revice_msg;
  u32 errnum=0;
  u32 total_num=0;
  u8 test_tx_data[8]={0};
  u8 test_rx_data[8]={0};
  u8 i=0;
  u8 button_key=0;

  //����ȫ�ֱ����ź���
  mico_rtos_init_mutex(&KvMutex);
  
  time_10ms_out_flag=0;
  kv_system_inital(&kv_sys_sate);
  I2c_slave_Inital(&powrboard);
  if(TRUE==check_slave_device_on_line(&powrboard))
  {
     mico_log("slave is online......slave is online------slave is online---slave is online");  
  }
  if(kNoErr==mico_init_timer(&kv_timer, 10, kv_timer_handle,NULL))//����10ms��ʱ��
  {
    mico_log("the timer of kv is created sucessed");  

  }
  if(kNoErr==mico_start_timer(&kv_timer))
  {
     mico_log("start the timer of kv is sucessed");
  }
#if 0  
  if(kNoErr== OsAslMsgQueueCreate(&KV_QUE, "KV_QUEUE", 1))
  {
     mico_log("creat the msg queue sucess");
  }
#endif
  if(kNoErr==mico_system_notify_register(mico_notify_WIFI_SCAN_COMPLETED,(void *)scan_wifi_complete_call_back,NULL))
  {
      mico_log("add the wifi scan note fication sucess");
  }

  proces_init_out_dis(&powrboard);

  for(;;)
  {
	 
   if(time_10ms_out_flag)
   {
     time_10ms_out_flag=0;
	 time_dis(&powrboard);
     mico_start_timer(&kv_timer);
   }
   else
   {
     mico_thread_msleep(10);
   }
#if 0   
	 if(test_queue_flag==1)
	 {
		 if(kNoErr!=OsAslMsgQueueReadMsg(&KV_QUE, &revice_msg, 10))
		 {
	       
	        mico_log("read the que messge failed");
		 }
		 else
		 {
		    mico_log("read the que messge sucess---------------------sucess");
            mico_log("the key vaule is %d",revice_msg.Arg);
			mico_log("the key short long is %d",revice_msg.Arg2);
		 }
		 test_queue_flag=0;
	 }
#endif
	 
  }
}


/*
  *Function name: Start_Kv_Task
  *Descritpion:��ȡ�̻�״̬���Լ�����������״̬
  *input:

  *output:
  
*/

void KV_SYSTEM_STATE_Update(KV_SYSTEM_STATE *kv_system_state)
{
  KvVarProtect(TRUE);
  memcpy(&kv_sys_sate,kv_system_state,sizeof(KvState));
  STATE_BUZ=kv_system_state->buzz_state;
  KvVarProtect(FALSE);   
  return ;
}

void KV_SYSTEM_FAN_TIME_CLEAN(void)
{
   SlaveMessge messge_tmp_fan_time;
   messge_tmp_fan_time.id=ID_CLR_CLEAN_TIME;
   messge_tmp_fan_time.value=0;
   if(FALSE==Set_Slave_Vaule(&powrboard,&messge_tmp_fan_time))
   {
      mico_log("clean the fan time faulse");
   }

}



/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------�������ʱ���ʱ-----------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/	
void    fan_time_dis(mico_i2c_device_t * slave_device)
{
  SlaveMessge messge_tmp;
  KvVarProtect(TRUE);  
  if(kv_sys_sate.STATE_WIND)
  {
    TIME.TIME_FAN_10MS++;
    if(TIME.TIME_FAN_10MS>=6000)//6000--------------------------------------
    {
      TIME.TIME_FAN_10MS=0;
      TIME.TIME_FAN_1F++;
      if(TIME.TIME_FAN_1F>=60)//60---------------------------------------------
      {
        TIME.TIME_FAN_1F=0;
        messge_tmp.id=ID_READ_CLEAN_TIME;
        messge_tmp.value=0x00;
        kv_sys_sate.STATE_FAN_1H=Read_Slave_Vaule(slave_device,&messge_tmp);
        if(kv_sys_sate.STATE_FAN_1H!=FALSE)
        {
          mico_log("the clean time is %d",kv_sys_sate.STATE_FAN_1H);
        }
        else
        {
          mico_log("read the clean time failed-------------------------failed");
        }
        /**************************************************/           
      }
    }   
  }
  KvVarProtect(FALSE);
}



/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//----------�������ʱ���ʱ-----------
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/	
void Heart_time_dis(mico_i2c_device_t * slave_device)
{
    SlaveMessge messge_tmp;
	u8 heart_vaule=0;
    TIME.TIME_Heart_10MS++;
    if(TIME.TIME_Heart_10MS>=6000)//6000--------------------------------------
    {
    
		TIME.TIME_Heart_10MS=0;
		messge_tmp.id=ID_READ_SET_HAERT;
		messge_tmp.value=0xaa;
		heart_vaule=Read_Slave_Vaule(slave_device,&messge_tmp);
		if(heart_vaule!=FALSE && heart_vaule==0xaa)
		{
		  mico_log("the deive of the power board online\r\n");
		}
		else
		{
           mico_log("read the heart value failed or the powerboad heart commucaiton is wrong-------------------------failed");
		}
 
 /**************************************************/           
     }
}



/*
  *Function name: Start_Kv_Task
  *Descritpion:��ȡ�̻�״̬���Լ�����������״̬
  *input:

  *output:
  
*/

void KV_SYSTEM_STATE_Read(KV_SYSTEM_STATE *kv_system_state)
{
  KvVarProtect(TRUE);
  
  memcpy(kv_system_state,&kv_sys_sate,sizeof(KvState));
  kv_system_state->buzz_state=STATE_BUZ;
  KvVarProtect(FALSE);   
  
  return ;
}


/*
  *Function name: Start_Kv_Task
  *Descritpion:�����̻�����
  *input:

  *output:
  
*/
void StartKvTask(void *arg)
{
  mico_rtos_create_thread(&KvSsytemTb, MICO_APPLICATION_PRIORITY, "KV_TASK", Kv_Task, STACK_SIZE_USER_MAIN_THREAD, arg);
}




