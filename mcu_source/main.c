//功能：实现串口接收数据后赋值
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
	
/*---------声明---------*/
uint8_t hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART);
uint8_t Wave_Gen_Config(uint8_t CHANNEL_NUM , struct_Wave_Config Wave_Config_Value);
uint8_t Wave_Para_Assign(uint8_t CHANNEL_NUM);
//extern int wave_data[64];
uint8_t Normal_Wave_Para_Assign(uint8_t CHANNEL_NUM);
/*---------定义---------*/
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
			
#define  DEFAULT_WAVE      0x00   //默认波形
#define  REPEAT_WAVE       0x01   //重复脉冲
#define  INTERRUPT_WAVE    0x02   //间歇性重复脉冲
#define  VARIED_WAVE       0x03   //扫频及缓升缓降波形
#define  SELF_DEFINED_WAVE 0x04   //自定义

#define  VARIED_WAVE_STOP_STATE    0x00
#define  VARIED_WAVE_RISING_STATE  0x01
#define  VARIED_WAVE_HOLD_STATE    0X02
#define  VARIED_WAVE_DOWN_STATE    0X03

/*---------变量---------*/
//global variable  全局
uint8_t recv_buff[20] = {0}; //存储接收到的命令及参数数据
uint8_t Channel_Sel;         //所有的数据都要先确定通道号！！！
uint8_t timer0_1s_occurred=0;
uint8_t timer0_1m_occurred=0;
uint8_t timer0_1h_occurred=0;
uint32_t timer1_1ms_occurred=0;
uint32_t timer1_channel_count[4]={0};  //用于计数波形使用到的时间，如断续波形的停止时间计数

//struct_Wave_Config Wave_Config_Get[4] ;  //获取到的config值放在这里
uint8_t CHANNEL_TIMING_FLAG[4]={0};
uint32_t INTERRUPT_WAVE_STOP_T[4] = {0};  //断续波形的停顿时间
uint32_t VARIED_WAVE_RAMP_T[4] = {0};     //EMS波形爬坡时长
uint32_t VARIED_WAVE_DOWN_T[4] = {0};     //EMS下坡时长
uint32_t VARIED_WAVE_HOLD_T[4] = {0};     //EMS保持时长
uint32_t VARIED_WAVE_STOP_T[4] = {0};     //EMS间隔时长
uint32_t WAVE_RUN_TOTAL[4] = {0} ;        //当前刺激运行时间

//flag variable  标志类
uint8_t start_drv[4] = {0};   //刺激通道中断中的标识
uint32_t wg_drv_irq_occurred[4] = {0}; //刺激通道中断次数统计
uint8_t err_code[4] = {0};  //中断中的错误标记
//INTERRUPT_WAVE 波形的一些标志参数
uint8_t INTERRUPT_WAVE_TIMIING_FLAG[4] = {0};  //完成一组刺激后需要开启此定时开关，计时结束需要关闭此计时开关

//EMS波形的一些标志参数
uint8_t VARIED_WAVE_STATE_FLAG= 1;
uint8_t UPDATA_ISEL_FLAG = 0;
uint8_t VARIED_WAVE_RUNNING_FLAG=0;  //非静止状态， 运行状态
uint8_t VARIED_WAVE_TIMIING_FLAG[4] = {0};     //完成一组刺激后需要开启此定时开关

void My_Ack(uint8_t command,uint8_t channel,uint8_t *value);


//--------串口相关函数---------//
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
				
			//确定命令组和命令
			switch((recv_buff[2] & 0xf0) >> 4)		//命令组
			{
				case 1:    //版本号
					break;
				
				case 0x2:	//启动停止命令						
					Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG  = recv_buff[4];
					break;				
				case 0x3:   //已没用，保留
					break;
				case 0x4:   //波形类型，根据波形类型，确定赋值方式
					Channel_Reg[Channel_Sel].Waveform_type = recv_buff[4];			
					//如果是默认波形，则无法输入参数，直接启动现有波形 
					//分为5类  0： 默认波形：直接启动或关闭，不能编辑波形，且会显示对应的
				    //         1： 重复脉冲 
				    //         2： 间歇性重复脉冲
				    //         3： 扫频及缓升缓降波形
				    //         4： 自定义									
					break;		
				case 0x5:   //寄存器配置							  
					{
						switch ((recv_buff[2] & 0x0f))
						{												
							case 0x0:   ADDR_WG_DRV_CONFIG_REG_SAVE = recv_buff[4];             //CONFIG寄存器
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
											 wave_data_array[Channel_Sel].wave_data[i] = Channel_Reg[Channel_Sel].ADDR_WG_DRV_IN_WAVE_REG;//wave_data 保存了波形中的最大值
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
				case 0x6:   //缩略图参数
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
				

			
			My_Ack(recv_buff[2],recv_buff[3],&recv_buff[4]);  //应答
			//接收到数据后先保存在了结构体中然后再给寄存器赋值
			//1、先传config配置			
			if(Channel_Reg[Channel_Sel].Waveform_type != DEFAULT_WAVE)
			{
			hex_to_struct_wave_config(Channel_Sel, ADDR_WG_DRV_CONFIG_REG_SAVE);//在上位机中这里应该根据波形类型不同
			                                                                    //然后确定是自定义的config还是一个固定值！！！
			Wave_Gen_Config(Channel_Sel , Config_Reg[Channel_Sel]);		
			}
			//2、再传其他reg寄存器值
			Wave_Para_Assign(Channel_Sel);	
			
			
			//3、控制启动或停止，同时计时刺激总时长
			if(Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG == ENABLE)
			{	
				DRVA->WAVE_GEN_DRV_CTRL_REG = ENABLE;
				CHANNEL_TIMING_FLAG[Channel_Sel] = 1;   //启动后也启动定时计数 ，可以设置定时关机（注意是定时刺激时间，中途暂停不计入内）
			}
			
			if(Channel_Reg[Channel_Sel].ADDR_WG_DRV_CTRL_REG == DISABLE)
			{	
				DRVA->WAVE_GEN_DRV_CTRL_REG =  DISABLE;
				CHANNEL_TIMING_FLAG[Channel_Sel] = 0;
				//printf("Channel %d disable!\n",Channel_Sel);
			}
			
			//printf("tpye = %d\n",Channel_Reg[Channel_Sel].Waveform_type);
			//清空buff里的数，继续存下一个数
			memset(recv_buff,0,20);
			i=0;
		}
}





/* --------------------------------------------------------------- */
//  函数名：My_Ack(uint8_t ret)
//	形参	：接收到的命令组和命令参数组成的8位的数据   通道号  值
//	返回值：无
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

/*------------中断-------------*/
/*
串口中断处理函数
功能：1、保存所有传入的参数并返回应答数据
      2、判断波形类型，分为（1）连续型（2）断续型（3）EMS型
      3、根据输入的波形类型，置对应参数的标志位
*/
void UART1_Handler(void)
{
	NVIC_ClearPendingIRQ(UART1_IRQn);//Clear Pending NVIC interrupt
	uint8_t rev_data = 0;
	rev_data = CMSDK_UART1->RBR;		
	My_Call_Back(rev_data);
    return;
}



/*波形发生器中断处理函数*/
/*
可以通过数组 wg_drv_irq_occurred 获取到中断发生的次数，每个通道每个周期发生的中断次数为 4 次
*/
void  WG_DRV_Handler(void) 
{ 
	CMSDK_WAVE_GEN_TypeDef *DRVA;
	//在运行过程中要判断发生的是哪一路的中断，
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
		//波形控制
		if(wg_drv_irq_occurred[i]%4==0)
		{
		switch(Channel_Reg[i].Waveform_type)
		{
				case INTERRUPT_WAVE:       //波形个数等于中断次数*4，定时器计数
					if((Channel_Wave_Thumb[i].pulse_quantity *4) == wg_drv_irq_occurred[i])  //产生N个周期波形
					{   
						DRVA->WAVE_GEN_DRV_CTRL_REG = DISABLE;
						DRVA->WAVE_GEN_DRV_INT_REG = 0x0;
						INTERRUPT_WAVE_TIMIING_FLAG[i] = ENABLE;  //开始计时标志
						wg_drv_irq_occurred[i] = 0;
					}
				break;
					
				//缓升缓降波形 
				
		
				case VARIED_WAVE: 	
					switch (VARIED_WAVE_STATE_FLAG)  //00 停止  01 上升  02 保持   03 下降
					{
						case VARIED_WAVE_RISING_STATE:	
							//第一组波形都是0档 / 0档 ，直到脉冲数 = 重复次数为止 
						
							if(wg_drv_irq_occurred[i]*4 >=  struct_EMS_Para[i].rising_timeQuotients)   //脉冲个数 = 单个脉冲重复次数，此时需要增大电流挡位
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
										VARIED_WAVE_STATE_FLAG = VARIED_WAVE_HOLD_STATE;  //切换为保持阶段
									//	wg_drv_irq_occurred[i] = 0;                       //波形中断次数计数清零
									}									
								}
								wg_drv_irq_occurred[i] = 0;                       //波形中断次数计数清零
							}
							break ;
							
						case VARIED_WAVE_HOLD_STATE:
							DRVA->WAVE_GEN_DRV_ISEL_REG = struct_EMS_Para[i].unit_current_for_EMS[32];
							for(int j=0;j<64;j++)
							{	
								DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = j;  
								DRVA->WAVE_GEN_DRV_IN_WAVE_REG = struct_EMS_Para[i].class_current_for_EMS[32]; 
							}
							
							if(wg_drv_irq_occurred[i]*4 >= struct_EMS_Para[i].time_of_stayQuotients)   //脉冲个数 = 单个脉冲重复次数，此时需要增大电流挡位
							{	
								
								UPDATA_ISEL_FLAG = 1;   //可以更新	
								VARIED_WAVE_STATE_FLAG = VARIED_WAVE_DOWN_STATE;  //切换为下降阶段
								wg_drv_irq_occurred[i]=0;
								struct_EMS_Para[i].Current_Current_Gear--;  //从31开始减到0为止！！！
							}					
						break;
						
						case VARIED_WAVE_DOWN_STATE:		
							if(wg_drv_irq_occurred[i]*4 >= struct_EMS_Para[i].down_timeQuotients)   //脉冲个数 = 单个脉冲重复次数，此时需要增大电流挡位
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
									wg_drv_irq_occurred[i] = 0;                       //波形中断次数计数清零
								}
								if(struct_EMS_Para[i].Current_Current_Gear < 0)
								{	
									VARIED_WAVE_STATE_FLAG = VARIED_WAVE_STOP_STATE;  //切换为静止阶段
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


/*按键中断处理函数*/



/*
定时器中断处理函数
功能：（1）连续型的波形用时计时
      （2）断续型的波形停止时长计时 以及 总刺激时长计时
	  （3）EMS波形的最高点保持时长计时 、 停止时间计时 以及 总刺激时长计时
*/


void TIMER0_Handler(void){ 
	CMSDK_TIMER0->INTCLEAR = 1;
	//中断一次计数1秒
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
	
    
	for(int i=0;i<4;i++)    //计时开关
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
		
		if(INTERRUPT_WAVE_TIMIING_FLAG[channel_num] == ENABLE)  //断续波形的停止阶段的时间计数使能
		{
			timer1_channel_count[channel_num]++;
			if(timer1_channel_count[channel_num] >= Channel_Wave_Thumb[channel_num].interval_time*100)   //传参单位秒，这里要转换为ms
			{			
				
				INTERRUPT_WAVE_TIMIING_FLAG[channel_num] = DISABLE;  //停止时间到，重新启动刺激输出
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






/*-------------初始化函数--------*/
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

//定时器初始化，形参位单位时间，单位秒
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


//将传来的原始config数据转换到config结构体中对应的位,写在uart中断处理函数中
uint8_t hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART){
	Config_Reg[CHANNEL_NUM].rest_en      =  (CONFIG_FROM_UART  & 0x1)>>0 ;
	Config_Reg[CHANNEL_NUM].negtive_en   =  (CONFIG_FROM_UART  & 0x2)>>1 ;
	Config_Reg[CHANNEL_NUM].silent_en    =  (CONFIG_FROM_UART  & 0x4)>>2 ;
	Config_Reg[CHANNEL_NUM].source_B_en  =  (CONFIG_FROM_UART  & 0x8)>>3 ; 
	Config_Reg[CHANNEL_NUM].continue_en  =  (CONFIG_FROM_UART  & 0x20)>>5 ;
	Config_Reg[CHANNEL_NUM].multi_electrode_en  =  (CONFIG_FROM_UART  & 0x40)>>6 ;
	return CONFIG_FROM_UART;
}
//此函数执行完后，相当于已经向config寄存器赋值了，因此在Config_Reg[i]中有对应通道的config参数值，但是初始化的数据使用的是
//初始化结构体时的数据

//config寄存器赋值
//此函数放在uart中用于接收串口发来的各个通道的config寄存器的数据，上位机中应该注明config各有效位的作用！！！ 重要！！！
//配置完config寄存器后需要根据使能位确定最终输入到其他寄存器的数据是否置0，如rest的时间是否生效和config 的bit0 有关，如果config的bit0为0，
//则rest输入的数值直接置0
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

//给四个通道的寄存器赋初始值,这里是固定的，在while前完成配置
uint8_t Channel_Reg_Init_Assign(uint8_t channel_num)
{     
	//配置初始的config寄存器 
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
	DRVA->WAVE_GEN_DRV_NEG_SCALE_REG        = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_NEG_SCALE_REG;   //这里需要注意溢出问题
	DRVA->WAVE_GEN_DRV_NEG_OFFSET_REG       = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_NEG_OFFSET_REG;  //这里需要注意溢出问题
	DRVA->WAVE_GEN_DRV_CLK_FREQ_REG         = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_CLK_FREQ_REG;
	DRVA->WAVE_GEN_DRV_DELAY_LIM_REG        = Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_DELAY_LIM_REG;	
	for(int i = 0 ; i < 64 ; i++){
		DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = i;  
		DRVA->WAVE_GEN_DRV_IN_WAVE_REG = wave_data_array[CHANNEL_NUM].wave_data[i]; 
	}
	//在赋值之后，需要对该清零的寄存器清零，重新赋值！！！
	if(Channel_Reg[CHANNEL_NUM].Waveform_type == INTERRUPT_WAVE)
	{
		DRVA->WAVE_GEN_DRV_INT_REG = 0x001f0001;  //这里使用固定的参数，不从上位机传来，但是自定义模式可以传来！！！	
	}
	
	if(Channel_Reg[CHANNEL_NUM].Waveform_type == VARIED_WAVE)  //EMS波形 缓升缓降效果
	{	
		DRVA->WAVE_GEN_DRV_ISEL_REG = 0;
		for(int i = 0 ; i < 64 ; i++){
			DRVA->WAVE_GEN_DRV_IN_WAVE_ADDR_REG = i;  
			DRVA->WAVE_GEN_DRV_IN_WAVE_REG = 0x00; 
		}		
		DRVA->WAVE_GEN_DRV_INT_REG = 0x001f0001; //使用固定的
		//启动这一通道后根据脉冲数量参数和发生中断次数做比较，确定当前已产生的波形数量	
	}
	return CHANNEL_NUM;
}

//根据config寄存器填其他值（给四个通道的寄存器赋值），定义的config结构体是：Config_Reg[0-4]
//                                                  此结构体实在uart中断函数接收到config参数后，通过函数
//                                                  hex_to_struct_wave_config(uint8_t CHANNEL_NUM , uint8_t CONFIG_FROM_UART)；
//                                                  Wave_Gen_Config(uint8_t CHANNEL_NUM , struct_Wave_Config Wave_Config_Value)
//                                                  赋值的！！！
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
	
    //Channel_Reg[];  此结构体先从uart中断处理函数接收传来的数据，然后在此处根据config参数处理，然后再给deiverA 赋值
    //rest_en  使能后，REST_T 时长不能等于0 ，否则不输出波形
	if(Config_Reg[CHANNEL_NUM].rest_en == DISABLE)
	{
		Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_REST_T_REG = 0;
	}

	
    if(Config_Reg[CHANNEL_NUM].silent_en == DISABLE)
	{
		Channel_Reg[CHANNEL_NUM].ADDR_WG_DRV_SILENT_T_REG = 0;
	}

	
	//根据波形类型的不同，传不同的参数
	switch (Channel_Reg[CHANNEL_NUM].Waveform_type)
	{
		case DEFAULT_WAVE:       //默认波形，将默认的参数传入，并需要统计刺激总时长
			Channel_Reg_Init_Assign(CHANNEL_NUM);
		break;
		
		case REPEAT_WAVE:        //重复波形，赋值后等待启动即可，并需要统计刺激总时长
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		break;

		case INTERRUPT_WAVE:     //断续波形，确认波形重复个数以及间隔时长，并需要统计刺激总时长
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		break;
		
		case VARIED_WAVE:	     //变化波形，以EMS为例，传入时长及基础波形参数，间隔时长 ，并统计刺激总时长
			Normal_Wave_Para_Assign(CHANNEL_NUM);
		    EMS_Wave_Para_Cal(CHANNEL_NUM,Channel_Wave_Thumb[CHANNEL_NUM], Channel_Reg[CHANNEL_NUM]);
		break;
		
		case SELF_DEFINED_WAVE:  //自定义波形，根据传入的参数绘制波形 ，自定义暂时不去实现！！！
			
		break;
	}
	
	return CHANNEL_NUM;
}


  

/*-------------主函数-----------*/
/*
主函数
功能：初始化及默认波形的参数赋值
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
	//启停、赋值功能放在uart中断中实现，while没有主要功能
	while(1){	
		for(int i = 0;  i<10000 ; i++){};
	}
	return 0;
}


