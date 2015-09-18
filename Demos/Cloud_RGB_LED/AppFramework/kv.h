/**
     Copyright (C) 2014 Legent Ltd.
    All rights reserved
  ***************************************************************************************
  *This file is the head file of i2c module
  ***************************************************************************************
*/ 

#ifndef _KV_H_
//#define _KV_H_
//#include "sysdefs.h"
#include "MicoDriverGpio.h"
#include "platform.h"
#include "MicoDriverI2c.h"
#include "mico_wlan.h"
#include "mico_system.h"
//#include "micodefine.h"

#define u8   unsigned char
#define u16  unsigned short int


#define KEY_SHORT 0
#define KEY_LONG  1

#define buzz_silent          0x00
#define buzz_one_sing        0x01
#define buzz_system_on       0x02
#define buzz_system_off      0x03 
#define buzz_system_power_on    0x04 //������4������״̬ 

#define baochaochange 7

typedef  struct 						//����������
{
  u16	KEY_KEEP ;	//�а���������
  u16	KEY_HIG;
  u8	KEY_LOW;
  u8	KEY_LIGHT;
  u16	KEY_NET;
  u8	KEY_POWER;
  u8	KEY_DATA;
}kv_key; //�̻�����



typedef struct						
{
	u8	STATE_WIND ;//01���٣�03���٣�00����
	u8  STATE_SYS;//ϵͳ״̬ 0 �ػ� 1���� 2��ʱ�ػ�
	u8	STATE_LIGHT;//1��������0������
	u8	STATE_NET;//�����״̬
	u8  STATE_FAN_1H;//
}KvState;//�̻�״̬



typedef struct						//����������
{
	u8	TIME_SHAN_10MS;//��˸��ʱ
	u8	TIME_SHAN2_10MS;//��˸��ʱ
	u8	TIME_SHAN3_10MS;//��˸��ʱ
	u16	TIME_YS_10MS;//1������ʱ��ʱ
	u16	TIME_SYS_10MS;//
	u16 TIME_BAOC_10MS;
	u16 TIME_FAN_10MS;//��ϴ10ms��ʱ
    u8  TIME_FAN_1F;//��ϴ���Ӽ�ʱ
    u16 TIME_Heart_10MS;//����10ms��ʱ
}KvTime; //�̻���ʱʱ��


typedef enum					//������
{
  kv_key_power=(u8)0x01,
  kv_key_low=(u8)0x02,
  kv_key_hig=(u8)0x03,
  kv_key_light=(u8)0x04,
  kv_key_net=(u8)0x05
  
}KvKeyId;




typedef struct					//������
{
	KvKeyId key_id;//����ID,��ֵ
	bool key_short_long;	
}KvKeyMessge; //�̻���ʱʱ��



typedef struct						//����������
{
	u8	id;
	u8  value;
	u8  sum_check;
	u8  xor_check;

}SlaveMessge; //�̻���ʱʱ��



typedef  struct 						//����������
{
 KvState kv_state;
 u8      buzz_state;//���÷���������״̬
}KV_SYSTEM_STATE;


typedef __packed struct{
    uint8_t* pMsgData;
    uint32_t MsgDataSize;
    uint8_t* pMsgData2;
    uint32_t MsgDataSize2;    
    uint16_t MsgId;
    int Arg;/* used as user defined */
    int Arg2;    
    signed long OrgTaskId;
    signed long DstTaskId;	
}OsAslMsgT;


void    fan_time_dis(mico_i2c_device_t * slave_device);
void	key_check(mico_i2c_device_t *slave_device);
void	time_shan_dis(void);
void	gjys_1f_dis(mico_i2c_device_t *slave_device);
void	out_dis(mico_i2c_device_t *slave_device);
void	time_dis(mico_i2c_device_t * slave_device);
void	baoc_dis(mico_i2c_device_t *slave_device);
void	fan_out_dis(mico_i2c_device_t *slave_device);
void    light_out_dis(mico_i2c_device_t *slave_device);
void	power_led_dis();
void	net_out_dis(void);
void    proces_out_dis(mico_i2c_device_t *slave_device);
void    Heart_time_dis(mico_i2c_device_t * slave_device);
void  scan_wifi_complete_call_back(ScanResult *wifi_si_result ,mico_Context_t * const context_tmp);


//the interface to the system
void KV_SYSTEM_STATE_Update(KV_SYSTEM_STATE *kv_system_state);
void KV_SYSTEM_STATE_Read(KV_SYSTEM_STATE *kv_system_state);
void KV_SYSTEM_FAN_TIME_CLEAN(void);





bool  check_slave_device_on_line(mico_i2c_device_t *slave_deive);
void  I2c_slave_Inital(mico_i2c_device_t *slave_deive);
void  I2c_slave_Io_Inital(void *arg);
bool  Set_Slave_Vaule(mico_i2c_device_t *slave_deive,SlaveMessge *messge);
u8    Read_Slave_Vaule(mico_i2c_device_t *slave_device,SlaveMessge *messge);

void  kv_timer_handle(void *arg);
void  seng_key_messge(KvKeyId kv_key_id,bool short_long);



void    kv_system_inital(KvState *kv_state);//inital the io  i2c_io ohter io nitaland the system stauste vaule
void    Kv_Task(void *arg);
void    Start_Kv_Task(void *arg);

#endif


