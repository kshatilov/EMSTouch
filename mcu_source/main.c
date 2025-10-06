//���ܣ�ʵ�ִ��ڽ������ݺ�ֵ
#include <stdio.h>
#include "uart_stdout.h"
#include "CMSDK_CM0.h"
#include "sawtooth.h"
#include "boost_select.h"
#include "crcLib.h"
#include "CMSDK_driver.h"
#include "string.h"
#include "data_define.h"

#define REG_FCR_ADDR  CMSDK_UART1_BASE + 0x0008
#define HW8_REG(ADDRESS)   (*((volatile unsigned char  *)(ADDRESS)))
	
/*---------����---------*/
uint8_t hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART);
uint8_t Wave_Gen_Config(uint8_t CHANNEL_NUM , struct_Wave_Config Wave_Config_Value);
uint8_t Wave_Para_Assign(uint8_t CHANNEL_NUM);
//extern int wave_data[64];
uint8_t Normal_Wave_Para_Assign(uint8_t CHANNEL_NUM);
/*---------����---------*/
#define ENABLE 1
#define DISABLE 0

#define DriverA_CH0 0x0
#define DriverA_CH1 0x1
#define DriverA_CH2 0x2
#define DriverA_CH3 0x3
 
#define HSI_FREQ_SEL_4MHZ     0x0
#define HSI_FREQ_SEL_8MHZ     0x1
#define HSI_FREQ_SEL_16MHZ    0x2
#define HSI_FREQ_SEL_32MHZ    0x3

#define UART0_PCLK_EN   0
#define UART1_PCLK_EN   1
#define SPI0_PCLK_EN    2
#define SPI1_PCLK_EN    3
#define I2C0_PCLK_EN    4
#define I2C1_PCLK_EN    5
#define WDT_PCLK_EN 	6
#define PWM_PCLK_EN 	7
#define TIMER0_PCLK_EN  8
#define TIMER1_PCLK_EN  9
#define DUAL_TIMER_PCLK_EN  10
#define LCD_PCLK_EN 		11
#define WAVE_GEN_PCLK_EN 	12
#define ADC_PCLK_EN 		13
#define ANALOG_PCLK_EN 		14
#define RTC_PCLK_EN 		15
			
#define  DEFAULT_WAVE      0x00   //Ĭ�ϲ���
#define  REPEAT_WAVE       0x01   //�ظ�����
#define  INTERRUPT_WAVE    0x02   //��Ъ���ظ�����
#define  VARIED_WAVE       0x03   //ɨƵ��������������
#define  SELF_DEFINED_WAVE 0x04   //�Զ���

#define  VARIED_WAVE_STOP_STATE    0x00
#define  VARIED_WAVE_RISING_STATE  0x01
#define  VARIED_WAVE_HOLD_STATE    0X02
#define  VARIED_WAVE_DOWN_STATE    0X03

/*---------����---------*/
//global variable  ȫ��
uint8_t recv_buff[20] = {0}; //�洢���յ��������������
uint8_t Channel_Sel;         //���е����ݶ�Ҫ��ȷ��ͨ���ţ�����
uint8_t timer0_1s_occurred=0;
uint8_t timer0_1m_occurred=0;
uint8_t timer0_1h_occurred=0;
uint32_t timer1_1ms_occurred=0;
uint32_t timer1_channel_count[4]={0};  //���ڼ�������ʹ�õ���ʱ�䣬��������ε�ֹͣʱ�����

//struct_Wave_Config Wave_Config_Get[4] ;  //��ȡ����configֵ��������
uint8_t CHANNEL_TIMING_FLAG[4]={0};
uint32_t INTERRUPT_WAVE_STOP_T[4] = {0};  //�������ε�ͣ��ʱ��
uint32_t VARIED_WAVE_RAMP_T[4] = {0};     //EMS��������ʱ��
uint32_t VARIED_WAVE_DOWN_T[4] = {0};     //EMS����ʱ��
uint32_t VARIED_WAVE_HOLD_T[4] = {0};     //EMS����ʱ��
uint32_t VARIED_WAVE_STOP_T[4] = {0};     //EMS���ʱ��
uint32_t WAVE_RUN_TOTAL[4] = {0} ;        //��ǰ�̼�����ʱ��

//flag variable  ��־��
uint8_t start_drv[4] = {0};   //�̼�ͨ���ж��еı�ʶ
uint32_t wg_drv_irq_occurred[4] = {0}; //�̼�ͨ���жϴ���ͳ��
uint8_t err_code[4] = {0};  //�ж��еĴ�����
//INTERRUPT_WAVE ���ε�һЩ��־����
uint8_t INTERRUPT_WAVE_TIMIING_FLAG[4] = {0};  //���һ��̼�����Ҫ�����˶�ʱ���أ���ʱ������Ҫ�رմ˼�ʱ����

//EMS���ε�һЩ��־����
uint8_t VARIED_WAVE_STATE_FLAG= 1;
uint8_t UPDATA_ISEL_FLAG = 0;
uint8_t VARIED_WAVE_RUNNING_FLAG=0;  //�Ǿ�ֹ״̬�� ����״̬
uint8_t VARIED_WAVE_TIMIING_FLAG[4] = {0};     //���һ��̼�����Ҫ�����˶�ʱ����

void My_Ack(uint8_t command,uint8_t channel,uint8_t *value);


//--------������غ���---------//
void My_Call_Back(uint8_t recv)
{		
		
		CMSDK_WAVE_GEN_TypeDef *DRVA;
		static uint8_t i=0;		
		if(recv == 0x55 && recv_buff[0] == 0){
			recv_buff[i]=0x55;
			i++;
		}		
		else if(recv_buff[0]==0x55 && recv == 0x55 && recv_buff[1] == 0 ){
			recv_buff[i]=0x55;
			i++;
		}
		else if(recv_buff[0]==0x55 && recv_buff[1] == 0x55 && i <9)
			recv_buff[i++] = recv;		
		
		if(i>=9 && recv_buff[8] == crc8_maxim(recv_buff, 8))	
		{	
			Channel_Sel = recv_buff[3];		
	       	if(Channel_Sel==0){
	        	DRVA= WAVE_GEN_DRVA_BLK0;		
	        }
	        else if(Channel_Sel==1){
	        	DRVA= WAVE_GEN_DRVA_BLK1;	
	        }
	        else if(Channel_Sel==2){
	        	DRVA= WAVE_GEN_DRVA_BLK2;	
	        }
	        else if(Channel_Sel==3){
	        	DRVA= WAVE_GEN_DRVA_BLK3;	
	        }
	        else 
	            DRVA= WAVE_GEN_DRVA_BLK0;	
				
			//ȷ�������������
			switch((recv_buff[2] & 0xf0) >> 4)		//������
			{
				case 1:    //�汾��
					break;
				
				case 0x2:	//����ֹͣ����						
					Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG  = recv_buff[4];
					break;				
				case 0x3:   //��û�ã�����
					break;
				case 0x4:   //�������ͣ����ݲ������ͣ�ȷ����ֵ��ʽ
					Channel_Reg[Channel_Sel].Waveform_type = recv_buff[4];			
					//�����Ĭ�ϲ��Σ����޷����������ֱ���������в��� 
					//��Ϊ5��  0�� Ĭ�ϲ��Σ�ֱ��������رգ����ܱ༭���Σ��һ���ʾ��Ӧ��
				    //         1�� �ظ����� 
				    //         2�� ��Ъ���ظ�����
				    //         3�� ɨƵ��������������
				    //         4�� �Զ���									
					break;		
				case 0x5:   //�Ĵ�������							  
					{
						switch ((recv_buff[2] & 0x0f))
						{												
							case 0x0:   ADDR_WG_DRV_CONFIG_REG_SAVE = recv_buff[4];             //CONFIG�Ĵ���
								break;
							case 0x1:   Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x2:   Channel_Reg[Channel_Sel].ADDR_WG_DRV_REST_T_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x3:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_SILENT_T_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x4:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_HLF_WAVE_PRD_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x5:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_NEG_HLF_WAVE_PRD_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x6:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_CLK_FREQ_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x7:	
								break;
							case 0x8:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_IN_WAVE_REG = *(uint32_t *)(&recv_buff[4]);
										for(int i = 0;i<64 ;i++)
										{
											 wave_data_array[Channel_Sel].wave_data[i] = Channel_Reg[Channel_Sel].ADDR_WG_DRV_IN_WAVE_REG;//wave_data �����˲����е����ֵ
										}
									   
								break;
							case 0x9:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_ALT_LIM_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0xA:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_ALT_SILENT_LIM_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0xB:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_DELAY_LIM_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0xC:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_NEG_SCALE_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0xD:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_NEG_OFFSET_REG = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0xE:	Channel_Reg[Channel_Sel].ADDR_WG_DRV_INT_REG = *(uint32_t *)(&recv_buff[4]);
								break;								
							case 0xF:   Channel_Reg[Channel_Sel].ADDR_WG_DRV_ISEL_REG = *(uint32_t *)(&recv_buff[4]);
								break;
						}								
					}					
					break;			
				case 0x6:   //����ͼ����
					{
						switch ((recv_buff[2] & 0x0f))
						{					
							case 0x0:   Channel_Wave_Thumb[Channel_Sel].total_time = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x1:   Channel_Wave_Thumb[Channel_Sel].interval_time = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x2:   Channel_Wave_Thumb[Channel_Sel].pulse_quantity = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x3:	Channel_Wave_Thumb[Channel_Sel].time_of_stay_at_high = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x4:	Channel_Wave_Thumb[Channel_Sel].rising_time = *(uint32_t *)(&recv_buff[4]);
								break;
							case 0x5:	Channel_Wave_Thumb[Channel_Sel].down_time = *(uint32_t *)(&recv_buff[4]);
								break;
						}								
					}
					break;
			}
				

			
			My_Ack(recv_buff[2],recv_buff[3],&recv_buff[4]);  //Ӧ��
			//���յ����ݺ��ȱ������˽ṹ����Ȼ���ٸ��Ĵ�����ֵ
			//1���ȴ�config����			
			if(Channel_Reg[Channel_Sel].Waveform_type != DEFAULT_WAVE)
			{
			hex_to_struct_wave_config(Channel_Sel, ADDR_WG_DRV_CONFIG_REG_SAVE);//����λ��������Ӧ�ø��ݲ������Ͳ�ͬ
			                                                                    //Ȼ��ȷ�����Զ����config����һ���̶�ֵ������
			Wave_Gen_Config(Channel_Sel , Config_Reg[Channel_Sel]);		
			}
			//2���ٴ�����reg�Ĵ���ֵ
			Wave_Para_Assign(Channel_Sel);	
			
			
			//3������������ֹͣ��ͬʱ��ʱ�̼���ʱ��
			if(Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG == ENABLE)
			{	
				DRVA->WAVE_GEN_DRV_CTRL_REG = ENABLE;
				CHANNEL_TIMING_FLAG[Channel_Sel] = 1;   //������Ҳ������ʱ���� ���������ö�ʱ�ػ���ע���Ƕ�ʱ�̼�ʱ�䣬��;��ͣ�������ڣ�
			}
			
			if(Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG == DISABLE)
			{	
				DRVA->WAVE_GEN_DRV_CTRL_REG =  DISABLE;
				CHANNEL_TIMING_FLAG[Channel_Sel] = 0;
				//printf("Channel %d disable!\n",Channel_Sel);
			}
			
			//printf("tpye = %d\n",Channel_Reg[Channel_Sel].Waveform_type);
			//���buff���������������һ����
			memset(recv_buff,0,20);
			i=0;
		}
}





/* --------------------------------------------------------------- */
//  ��������My_Ack(uint8_t ret)
//	�β�	�����յ�������������������ɵ�8λ������   ͨ����  ֵ
//	����ֵ����
/* --------------------------------------------------------------- */
void My_Ack(uint8_t command,uint8_t channel,uint8_t *value)
{
	uint8_t send_data[10] = {0};
	send_data[0] = 0x55;
	send_data[1] = 0x55;
	send_data[2] = 0x80 | command;
	send_data[3] = channel;
	*((uint32_t *)(&send_data[4])) = *((uint32_t *)(value));
	send_data[8] = crc8_maxim(send_data, 8);
	for(int i = 0;i < 9;i++)
		printf("%c",send_data[i]);
	
}

/*------------�ж�-------------*/
/*
�����жϴ�����
���ܣ�1���������д���Ĳ���������Ӧ������
      2���жϲ������ͣ���Ϊ��1�������ͣ�2�������ͣ�3��EMS��
      3����������Ĳ������ͣ��ö�Ӧ�����ı�־λ
*/
void UART1_Handler(void)
{
	NVIC_ClearPendingIRQ(UART1_IRQn);//Clear Pending NVIC interrupt
	uint8_t rev_data = 0;
	rev_data = CMSDK_UART1->RBR;		
	My_Call_Back(rev_data);
    return;
}



/*���η������жϴ�����*/
/*
����ͨ������ wg_drv_irq_occurred ��ȡ���жϷ����Ĵ�����ÿ��ͨ��ÿ�����ڷ������жϴ���Ϊ 4 ��
*/
void  WG_DRV_Handler(void) 
{ 
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	//�����й�����Ҫ�жϷ���������һ·���жϣ�
	for(int i =0; i<4;i++)
	{
		if(i==0){
			DRVA= WAVE_GEN_DRVA_BLK0;		
		}
		else if(i==1){
			DRVA= WAVE_GEN_DRVA_BLK1;	
		}
		else if(i==2){
			DRVA= WAVE_GEN_DRVA_BLK2;	
		}
		else if(i==3){
			DRVA= WAVE_GEN_DRVA_BLK3;	
		}
		
		if((DRVA->WAVE_GEN_DRV_INT_REG & CMSDK_WAVE_GEN_DRV_INT_READ_DRIVER_NUM_Msk) == i) //if driver 0 generated interrupt
		{
				if((DRVA->WAVE_GEN_DRV_INT_REG & CMSDK_WAVE_GEN_DRV_INT_FIRSTADDR_STS_Msk) == CMSDK_WAVE_GEN_DRV_INT_FIRSTADDR_STS_Msk) //clear 1st address interrupt together with address change to load the next half data
				{
						if(((DRVA->WAVE_GEN_DRV_INT_REG & CMSDK_WAVE_GEN_DRV_INT_READ_FIRST_ADDR_Msk)>>CMSDK_WAVE_GEN_DRV_INT_READ_FIRST_ADDR_Pos) == 0) // set first address 32; second address 63
						{
								DRVA->WAVE_GEN_DRV_INT_REG = (63 << CMSDK_WAVE_GEN_DRV_INT_SECOND_ADDR_Pos) | (32 << CMSDK_WAVE_GEN_DRV_INT_FIRST_ADDR_Pos) | CMSDK_WAVE_GEN_DRV_INT_FIRSTADDR_CLR_Msk | CMSDK_WAVE_GEN_DRV_INT_EN_Msk;
								start_drv[i] = 1;
						}
						else if(((DRVA->WAVE_GEN_DRV_INT_REG & CMSDK_WAVE_GEN_DRV_INT_READ_FIRST_ADDR_Msk)>>CMSDK_WAVE_GEN_DRV_INT_READ_FIRST_ADDR_Pos) == 32) // set first address 0; second address 31
						{
								DRVA->WAVE_GEN_DRV_INT_REG = (31 << CMSDK_WAVE_GEN_DRV_INT_SECOND_ADDR_Pos) | (0 << CMSDK_WAVE_GEN_DRV_INT_FIRST_ADDR_Pos) | CMSDK_WAVE_GEN_DRV_INT_FIRSTADDR_CLR_Msk | CMSDK_WAVE_GEN_DRV_INT_EN_Msk;
								start_drv[i] = 0;
						}
						wg_drv_irq_occurred[i]++; 
				}
				if((DRVA->WAVE_GEN_DRV_INT_REG & CMSDK_WAVE_GEN_DRV_INT_SECONDADDR_STS_Msk) == CMSDK_WAVE_GEN_DRV_INT_SECONDADDR_STS_Msk) //this interrupt shall never activated otherwise we missed chance to load the next half data
				{
						DRVA->WAVE_GEN_DRV_INT_REG = CMSDK_WAVE_GEN_DRV_INT_SECONDADDR_CLR_Msk | CMSDK_WAVE_GEN_DRV_INT_EN_Msk;
						err_code[i]++;
				}		
		}
		//���ο���
		if(wg_drv_irq_occurred[i]%4==0)
		{
		switch(Channel_Reg[i].Waveform_type)
		{
				case INTERRUPT_WAVE:       //���θ��������жϴ���*4����ʱ������
					if((Channel_Wave_Thumb[i].pulse_quantity *4) == wg_drv_irq_occurred[i])  //����N�����ڲ���
					{   
						DRVA->WAVE_GEN_DRV_CTRL_REG = DISABLE;
						DRVA->WAVE_GEN_DRV_INT_REG = 0x0;
						INTERRUPT_WAVE_TIMIING_FLAG[i] = ENABLE;  //��ʼ��ʱ��־
						wg_drv_irq_occurred[i] = 0;
					}
				break;
					
				//������������ 
				
		
				case VARIED_WAVE: 	
					switch (VARIED_WAVE_STATE_FLAG)  //00 ֹͣ  01 ����  02 ����   03 �½�
					{
						case VARIED_WAVE_RISING_STATE:	
							//��һ�鲨�ζ���0�� / 0�� ��ֱ�������� = �ظ�����Ϊֹ 
						
							if(wg_drv_irq_occurred[i]*4 >=  struct_EMS_Para[i].rising_timeQuotients)   //������� = ���������ظ���������ʱ��Ҫ���������λ
							{						
								if(struct_EMS_Para[i].Current_Current_Gear < 32)  // 0-32  
								{	
									DRVA->WAVE_GEN_DRV_ISEL_REG = struct_EMS_Para[i].unit_current_for_EMS[struct_EMS_Para[i].Current_Current_Gear];
									for(int j=0;j<64;j++)
									{	
										DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = j;  
										DRVA->WAVE_GEN_DRV_IN_WAVE_REG = struct_EMS_Para[i].class_current_for_EMS[struct_EMS_Para[i].Current_Current_Gear]; 
									}
									struct_EMS_Para[i].Current_Current_Gear++;
									if(struct_EMS_Para[i].Current_Current_Gear >= 32)
									{	
										VARIED_WAVE_STATE_FLAG = VARIED_WAVE_HOLD_STATE;  //�л�Ϊ���ֽ׶�
									//	wg_drv_irq_occurred[i] = 0;                       //�����жϴ�����������
									}									
								}
								wg_drv_irq_occurred[i] = 0;                       //�����жϴ�����������
							}
							break ;
							
						case VARIED_WAVE_HOLD_STATE:
							DRVA->WAVE_GEN_DRV_ISEL_REG = struct_EMS_Para[i].unit_current_for_EMS[32];
							for(int j=0;j<64;j++)
							{	
								DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = j;  
								DRVA->WAVE_GEN_DRV_IN_WAVE_REG = struct_EMS_Para[i].class_current_for_EMS[32]; 
							}
							
							if(wg_drv_irq_occurred[i]*4 >= struct_EMS_Para[i].time_of_stayQuotients)   //������� = ���������ظ���������ʱ��Ҫ���������λ
							{	
								
								UPDATA_ISEL_FLAG = 1;   //���Ը���	
								VARIED_WAVE_STATE_FLAG = VARIED_WAVE_DOWN_STATE;  //�л�Ϊ�½��׶�
								wg_drv_irq_occurred[i]=0;
								struct_EMS_Para[i].Current_Current_Gear--;  //��31��ʼ����0Ϊֹ������
							}					
						break;
						
						case VARIED_WAVE_DOWN_STATE:		
							if(wg_drv_irq_occurred[i]*4 >= struct_EMS_Para[i].down_timeQuotients)   //������� = ���������ظ���������ʱ��Ҫ���������λ
							{
								if(struct_EMS_Para[i].Current_Current_Gear >= 0)  // 0-32 
								{									
									DRVA->WAVE_GEN_DRV_ISEL_REG = struct_EMS_Para[i].unit_current_for_EMS[struct_EMS_Para[i].Current_Current_Gear];
									for(int j=0;j<64;j++)
									{	
										DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = j;  
										DRVA->WAVE_GEN_DRV_IN_WAVE_REG = struct_EMS_Para[i].class_current_for_EMS[struct_EMS_Para[i].Current_Current_Gear]; 
									}	
									struct_EMS_Para[i].Current_Current_Gear--;
									wg_drv_irq_occurred[i] = 0;                       //�����жϴ�����������
								}
								if(struct_EMS_Para[i].Current_Current_Gear < 0)
								{	
									VARIED_WAVE_STATE_FLAG = VARIED_WAVE_STOP_STATE;  //�л�Ϊ��ֹ�׶�
									VARIED_WAVE_TIMIING_FLAG[i] = 1;
									DRVA->WAVE_GEN_DRV_INT_REG     = 0;         
									DRVA->WAVE_GEN_DRV_CTRL_REG    = DISABLE;
								}								
							}		
						break;			
				}
			break;
			}
		}
	}
}


/*�����жϴ�����*/



/*
��ʱ���жϴ�����
���ܣ���1�������͵Ĳ�����ʱ��ʱ
      ��2�������͵Ĳ���ֹͣʱ����ʱ �Լ� �ܴ̼�ʱ����ʱ
	  ��3��EMS���ε���ߵ㱣��ʱ����ʱ �� ֹͣʱ���ʱ �Լ� �ܴ̼�ʱ����ʱ
*/


void TIMER0_Handler(void){ 
	CMSDK_TIMER0->INTCLEAR = 1;
	//�ж�һ�μ���1��
	timer0_1s_occurred++;
	
	if(timer0_1s_occurred == 60 )
	{ 
		timer0_1s_occurred  = 0;
		timer0_1m_occurred++;
	}
	
	if(timer0_1m_occurred == 60)
	{
		timer0_1m_occurred = 0;
		timer0_1h_occurred ++;
	}
	
	if(timer0_1h_occurred == 60)
	{
		timer0_1h_occurred = 0;
	}
	
    
	for(int i=0;i<4;i++)    //��ʱ����
	{
		if(CHANNEL_TIMING_FLAG[i]==1)
		{
			WAVE_RUN_TOTAL[i]++;
			//printf("channel - %d time - %d\n",i,WAVE_RUN_TOTAL[i]);
			if(WAVE_RUN_TOTAL[i] >= (60*15))
			{
		        if(i==0){
					WAVE_GEN_DRVA_BLK0->WAVE_GEN_DRV_CTRL_REG = 0x0;
				}
				else if(i==1){
					WAVE_GEN_DRVA_BLK1->WAVE_GEN_DRV_CTRL_REG = 0x0; 
				}
				else if(i==2){
					WAVE_GEN_DRVA_BLK2->WAVE_GEN_DRV_CTRL_REG = 0x0;	
				}
				else if(i==3){
					WAVE_GEN_DRVA_BLK3->WAVE_GEN_DRV_CTRL_REG = 0x0;	
				}
				WAVE_RUN_TOTAL[i] = 0;
				CHANNEL_TIMING_FLAG[i] = 0;
			}
		}
	}
}

void TIMER1_Handler(void){ 
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	CMSDK_TIMER1->INTCLEAR = 1;
	timer1_1ms_occurred++;
	
	for(int channel_num = 0 ; channel_num < 4 ;channel_num++)
	{	
		if(channel_num==0){
			DRVA= WAVE_GEN_DRVA_BLK0;		
		}
		else if(channel_num==1){
			DRVA= WAVE_GEN_DRVA_BLK1;	
		}
		else if(channel_num==2){
			DRVA= WAVE_GEN_DRVA_BLK2;	
		}
		else if(channel_num==3){
			DRVA= WAVE_GEN_DRVA_BLK3;	
		}
		
		if(INTERRUPT_WAVE_TIMIING_FLAG[channel_num] == ENABLE)  //�������ε�ֹͣ�׶ε�ʱ�����ʹ��
		{
			timer1_channel_count[channel_num]++;
			if(timer1_channel_count[channel_num] >= Channel_Wave_Thumb[channel_num].interval_time*100)   //���ε�λ�룬����Ҫת��Ϊms
			{			
				
				INTERRUPT_WAVE_TIMIING_FLAG[channel_num] = DISABLE;  //ֹͣʱ�䵽�����������̼����
				DRVA->WAVE_GEN_DRV_INT_REG     = 0x001f0001;         
				DRVA->WAVE_GEN_DRV_CTRL_REG    = ENABLE;	
				timer1_channel_count[channel_num] = 0 ; 
			}
		}
		
		if(VARIED_WAVE_TIMIING_FLAG[channel_num] == ENABLE)
		{
			timer1_channel_count[channel_num]++;
			if(timer1_channel_count[channel_num] >= Channel_Wave_Thumb[channel_num].interval_time*100)
			{
				VARIED_WAVE_TIMIING_FLAG[channel_num] = DISABLE;
				DRVA->WAVE_GEN_DRV_INT_REG     = 0x001f0001;  
				VARIED_WAVE_STATE_FLAG = VARIED_WAVE_RISING_STATE; 
				struct_EMS_Para[channel_num].Current_Current_Gear = 0;				
				DRVA->WAVE_GEN_DRV_CTRL_REG    = ENABLE;	
				timer1_channel_count[channel_num] = 0 ; 
			}
		}

		
	}
}






/*-------------��ʼ������--------*/
void WG_INT_init(void){
	NVIC_DisableIRQ(WG_DRV_IRQn);//Disable NVIC interrupt
    NVIC_ClearPendingIRQ(WG_DRV_IRQn);//Clear Pending NVIC interrupt
    NVIC_EnableIRQ(WG_DRV_IRQn);//Enable NVIC interrupt
}
uint8_t MTP_init(uint32_t MTP_CT_REG)
{
	CMSDK_MTPREG->MTP_CR = MTP_CT_REG;
	return  0;
}

uint8_t HSI_Clock_Freq_Init(uint8_t HSI_FREQ_SEL)
{
	CMSDK_SYSCON->HSI_CTRL = (CMSDK_SYSCON->HSI_CTRL & ~CMSDK_SYSCON_HSI_FREQ_Msk) | (HSI_FREQ_SEL << CMSDK_SYSCON_HSI_FREQ_Pos);
	return 0;
}


uint8_t PCLK_Enable(uint32_t APB_CLKEN_POS)
{
	CMSDK_SYSCON->APB_CLKEN |= 0x1 << APB_CLKEN_POS;
	return 0;
}

//��ʱ����ʼ�����β�λ��λʱ�䣬��λ��
float TIMER0_Init(float uint_time_second){
	NVIC_DisableIRQ(TIMER0_IRQn);//Disable NVIC interrupt	
    CMSDK_timer_Init(CMSDK_TIMER0, (uint32_t)(32000000*uint_time_second), 1);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn); 
	return uint_time_second;
}

float TIMER1_Init(float uint_time_ten_millisecond){
	NVIC_DisableIRQ(TIMER1_IRQn);//Disable NVIC interrupt	
    CMSDK_timer_Init(CMSDK_TIMER1, (uint32_t)(320000*uint_time_ten_millisecond), 1);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn); 
	return uint_time_ten_millisecond;
}


//��������ԭʼconfig����ת����config�ṹ���ж�Ӧ��λ,д��uart�жϴ�������
uint8_t hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART){
	Config_Reg[CHANNEL_NUM].rest_en      =  (CONFIG_FROM_UART  & 0x1)>>0 ;
	Config_Reg[CHANNEL_NUM].negtive_en   =  (CONFIG_FROM_UART  & 0x2)>>1 ;
	Config_Reg[CHANNEL_NUM].silent_en    =  (CONFIG_FROM_UART  & 0x4)>>2 ;
	Config_Reg[CHANNEL_NUM].source_B_en  =  (CONFIG_FROM_UART  & 0x8)>>3 ; 
	Config_Reg[CHANNEL_NUM].continue_en  =  (CONFIG_FROM_UART  & 0x20)>>5 ;
	Config_Reg[CHANNEL_NUM].multi_electrode_en  =  (CONFIG_FROM_UART  & 0x40)>>6 ;
	return CONFIG_FROM_UART;
}
//�˺���ִ������൱���Ѿ���config�Ĵ�����ֵ�ˣ������Config_Reg[i]���ж�Ӧͨ����config����ֵ�����ǳ�ʼ��������ʹ�õ���
//��ʼ���ṹ��ʱ������

//config�Ĵ�����ֵ
//�˺�������uart�����ڽ��մ��ڷ����ĸ���ͨ����config�Ĵ��������ݣ���λ����Ӧ��ע��config����Чλ�����ã����� ��Ҫ������
//������config�Ĵ�������Ҫ����ʹ��λȷ���������뵽�����Ĵ����������Ƿ���0����rest��ʱ���Ƿ���Ч��config ��bit0 �йأ����config��bit0Ϊ0��
//��rest�������ֱֵ����0
uint8_t Wave_Gen_Config(uint8_t CHANNEL_NUM , struct_Wave_Config Wave_Config_Value)
{   
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	if(CHANNEL_NUM==0){
		DRVA= WAVE_GEN_DRVA_BLK0;		
	}
	else if(CHANNEL_NUM==1){
		DRVA= WAVE_GEN_DRVA_BLK1;	
	}
	else if(CHANNEL_NUM==2){
		DRVA= WAVE_GEN_DRVA_BLK2;	
	}
	else if(CHANNEL_NUM==3){
		DRVA= WAVE_GEN_DRVA_BLK3;	
	}
    
	DRVA->WAVE_GEN_DRV_CONFIG_REG &=~ 0xff;
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= Wave_Config_Value.rest_en ;
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= (Wave_Config_Value.negtive_en  << 1);
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= (Wave_Config_Value.silent_en   << 2);
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= (Wave_Config_Value.source_B_en << 3);
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= (Wave_Config_Value.continue_en << 5);
	DRVA->WAVE_GEN_DRV_CONFIG_REG |= (Wave_Config_Value.multi_electrode_en <<6);
	
	return CHANNEL_NUM;
}

//���ĸ�ͨ���ļĴ�������ʼֵ,�����ǹ̶��ģ���whileǰ�������
uint8_t Channel_Reg_Init_Assign(uint8_t channel_num)
{     
	//���ó�ʼ��config�Ĵ��� 
		hex_to_struct_wave_config(channel_num, 0x7F);		
		Wave_Gen_Config(channel_num , Config_Reg[channel_num]);		
	    
		Channel_Reg[channel_num].ADDR_WG_DRV_REST_T_REG           = 50;
		Channel_Reg[channel_num].ADDR_WG_DRV_SILENT_T_REG         = 50 ;	
		Channel_Reg[channel_num].ADDR_WG_DRV_HLF_WAVE_PRD_REG     = 50 ;  
		Channel_Reg[channel_num].ADDR_WG_DRV_NEG_HLF_WAVE_PRD_REG = 50 ;

		Channel_Reg[channel_num].ADDR_WG_DRV_ISEL_REG             = 0x3 ;
		Channel_Reg[channel_num].ADDR_WG_DRV_NEG_SCALE_REG        = 1;
		Channel_Reg[channel_num].ADDR_WG_DRV_NEG_OFFSET_REG       = 0;
		Channel_Reg[channel_num].ADDR_WG_DRV_CLK_FREQ_REG         = 0x20;
		Channel_Reg[channel_num].ADDR_WG_DRV_INT_REG              = DISABLE;
		Channel_Reg[channel_num].ADDR_WG_DRV_DELAY_LIM_REG        = 0;
		for(int i = 0; i <64 ;i++){
			wave_data_array[channel_num].wave_data[i] = 0x66;			
		}
	
		Normal_Wave_Para_Assign(channel_num);				
		return channel_num;
}

uint8_t Normal_Wave_Para_Assign(uint8_t CHANNEL_NUM)
{
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	if(CHANNEL_NUM==0){
		DRVA= WAVE_GEN_DRVA_BLK0;		
	}
	else if(CHANNEL_NUM==1){
		DRVA= WAVE_GEN_DRVA_BLK1;	
	}
	else if(CHANNEL_NUM==2){
		DRVA= WAVE_GEN_DRVA_BLK2;	
	}
	else if(CHANNEL_NUM==3){
		DRVA= WAVE_GEN_DRVA_BLK3;	
	}

		
	DRVA->WAVE_GEN_DRV_ISEL_REG             = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_ISEL_REG;
	DRVA->WAVE_GEN_DRV_HLF_WAVE_PRD_REG     = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_HLF_WAVE_PRD_REG;
	DRVA->WAVE_GEN_DRV_NEG_HLF_WAVE_PRD_REG = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_NEG_HLF_WAVE_PRD_REG ;
	DRVA->WAVE_GEN_DRV_REST_T_REG           = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_REST_T_REG;
	DRVA->WAVE_GEN_DRV_SILENT_T_REG         = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_SILENT_T_REG;
	DRVA->WAVE_GEN_DRV_NEG_SCALE_REG        = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_NEG_SCALE_REG;   //������Ҫע���������
	DRVA->WAVE_GEN_DRV_NEG_OFFSET_REG       = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_NEG_OFFSET_REG;  //������Ҫע���������
	DRVA->WAVE_GEN_DRV_CLK_FREQ_REG         = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_CLK_FREQ_REG;
	DRVA->WAVE_GEN_DRV_DELAY_LIM_REG        = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_DELAY_LIM_REG;	
	for(int i = 0 ; i < 64 ; i++){
		DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = i;  
		DRVA->WAVE_GEN_DRV_IN_WAVE_REG = wave_data_array[CHANNEL_NUM].wave_data[i]; 
	}
	//�ڸ�ֵ֮����Ҫ�Ը�����ļĴ������㣬���¸�ֵ������
	if(Channel_Reg[CHANNEL_NUM].Waveform_type == INTERRUPT_WAVE)
	{
		DRVA->WAVE_GEN_DRV_INT_REG = 0x001f0001;  //����ʹ�ù̶��Ĳ�����������λ�������������Զ���ģʽ���Դ���������	
	}
	
	if(Channel_Reg[CHANNEL_NUM].Waveform_type == VARIED_WAVE)  //EMS���� ��������Ч��
	{	
		DRVA->WAVE_GEN_DRV_ISEL_REG = 0;
		for(int i = 0 ; i < 64 ; i++){
			DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = i;  
			DRVA->WAVE_GEN_DRV_IN_WAVE_REG = 0x00; 
		}		
		DRVA->WAVE_GEN_DRV_INT_REG = 0x001f0001; //ʹ�ù̶���
		//������һͨ��������������������ͷ����жϴ������Ƚϣ�ȷ����ǰ�Ѳ����Ĳ�������	
	}
	return CHANNEL_NUM;
}

//����config�Ĵ���������ֵ�����ĸ�ͨ���ļĴ�����ֵ���������config�ṹ���ǣ�Config_Reg[0-4]
//                                                  �˽ṹ��ʵ��uart�жϺ������յ�config������ͨ������
//                                                  hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART)��
//                                                  Wave_Gen_Config(uint8_t CHANNEL_NUM , struct_Wave_Config Wave_Config_Value)
//                                                  ��ֵ�ģ�����
uint8_t Wave_Para_Assign(uint8_t CHANNEL_NUM)
{	
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	if(CHANNEL_NUM==0){
		DRVA= WAVE_GEN_DRVA_BLK0;		
	}
	else if(CHANNEL_NUM==1){
		DRVA= WAVE_GEN_DRVA_BLK1;	
	}
	else if(CHANNEL_NUM==2){
		DRVA= WAVE_GEN_DRVA_BLK2;	
	}
	else if(CHANNEL_NUM==3){
		DRVA= WAVE_GEN_DRVA_BLK3;	
	}
	
    //Channel_Reg[];  �˽ṹ���ȴ�uart�жϴ��������մ��������ݣ�Ȼ���ڴ˴�����config��������Ȼ���ٸ�deiverA ��ֵ
    //rest_en  ʹ�ܺ�REST_T ʱ�����ܵ���0 �������������
	if(Config_Reg[CHANNEL_NUM].rest_en == DISABLE)
	{
		Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_REST_T_REG = 0;
	}

	
    if(Config_Reg[CHANNEL_NUM].silent_en == DISABLE)
	{
		Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_SILENT_T_REG = 0;
	}

	
	//���ݲ������͵Ĳ�ͬ������ͬ�Ĳ���
	switch (Channel_Reg[CHANNEL_NUM].Waveform_type)
	{
		case DEFAULT_WAVE:       //Ĭ�ϲ��Σ���Ĭ�ϵĲ������룬����Ҫͳ�ƴ̼���ʱ��
			Channel_Reg_Init_Assign(CHANNEL_NUM);
		break;
		
		case REPEAT_WAVE:        //�ظ����Σ���ֵ��ȴ��������ɣ�����Ҫͳ�ƴ̼���ʱ��
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		break;

		case INTERRUPT_WAVE:     //�������Σ�ȷ�ϲ����ظ������Լ����ʱ��������Ҫͳ�ƴ̼���ʱ��
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		break;
		
		case VARIED_WAVE:	     //�仯���Σ���EMSΪ��������ʱ�����������β��������ʱ�� ����ͳ�ƴ̼���ʱ��
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		    EMS_Wave_Para_Cal(CHANNEL_NUM,Channel_Wave_Thumb[CHANNEL_NUM], Channel_Reg[CHANNEL_NUM]);
		break;
		
		case SELF_DEFINED_WAVE:  //�Զ��岨�Σ����ݴ���Ĳ������Ʋ��� ���Զ�����ʱ��ȥʵ�֣�����
			
		break;
	}
	
	return CHANNEL_NUM;
}


  

/*-------------������-----------*/
/*
������
���ܣ���ʼ����Ĭ�ϲ��εĲ�����ֵ
*/
int main(void){
	MTP_init(0x00000002);	
	HSI_Clock_Freq_Init(HSI_FREQ_SEL_32MHZ);
	CMSDK_SYSCON->APB_CLKEN &=~ 0x7FFFF ; 
    PCLK_Enable(UART1_PCLK_EN);
	PCLK_Enable(TIMER0_PCLK_EN);
	PCLK_Enable(TIMER1_PCLK_EN);
	PCLK_Enable(WAVE_GEN_PCLK_EN);
	PCLK_Enable(ANALOG_PCLK_EN);
	Boost_Voltage_Sel(VOLTAGE_45V);
	WG_INT_init();
	UartStdOutInit();	
	TIMER0_Init(1);
	TIMER1_Init(1);
	//��ͣ����ֵ���ܷ���uart�ж���ʵ�֣�whileû����Ҫ����
	while(1){	
		for(int i = 0;  i<10000 ; i++){};
	}
	return 0;
}


