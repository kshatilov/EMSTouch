//功能：定义四个通道用到的数组，通用变量等
#ifndef DATA_DEFINE_H
#define DATA_DEFINE_H
#include <stdio.h>
#include "CMSDK_CM0.h"


//波形地址对应的电流幅值数组

struct struct_wave_data{
	int wave_data[64];
};
struct struct_wave_data wave_data_array[4];

//CONFIG寄存器内容的结构体，在配其他的寄存器的参数之前
//需要先确定config的内容

uint8_t ADDR_WG_DRV_CONFIG_REG_SAVE =0x00;
typedef struct Wave_Config{
	uint8_t rest_en;      //死区使能 
	uint8_t negtive_en;   //负半周波形使能
	uint8_t silent_en;    //静默使能
	uint8_t source_B_en;  //source_B 使能
	uint8_t continue_en;
	uint8_t multi_electrode_en;
}struct_Wave_Config;

struct_Wave_Config Config_Reg[4]; //用于保存串口发来的config数据（需要将原16进制数据转换到此结构体中）


//1 保存波形发生器的寄存器数据的结构体（1）
typedef struct Waveform_Register{
		uint8_t Waveform_type;	 
		uint32_t ADDR_WG_DRV_CTRL_REG;
		uint32_t ADDR_WG_DRV_REST_T_REG;
		uint32_t ADDR_WG_DRV_SILENT_T_REG;
		uint32_t ADDR_WG_DRV_HLF_WAVE_PRD_REG;
		uint32_t ADDR_WG_DRV_NEG_HLF_WAVE_PRD_REG;
		uint32_t ADDR_WG_DRV_CLK_FREQ_REG;
		uint32_t ADDR_WG_DRV_IN_WAVE_ADDR_REG;
		uint32_t ADDR_WG_DRV_IN_WAVE_REG;
		uint32_t ADDR_WG_DRV_ALT_LIM_REG;
		uint32_t ADDR_WG_DRV_ALT_SILENT_LIM_REG;
		uint32_t ADDR_WG_DRV_DELAY_LIM_REG;
		uint32_t ADDR_WG_DRV_NEG_SCALE_REG;
		uint32_t ADDR_WG_DRV_NEG_OFFSET_REG;
		uint32_t ADDR_WG_DRV_INT_REG;
		uint32_t ADDR_WG_DRV_ISEL_REG;       
		uint32_t ADDR_WG_DRV_SW_CONFIG_REG;		
		uint32_t ISEL_uA ; 
}struct_Wave_Reg;


struct_Wave_Reg Channel_Reg_Init[4];   //初始配置，需要提前赋好值
struct_Wave_Reg Channel_Reg[4]={0};        //接收参数后赋值



/*
2 保存缩略图时间参数的数组
分类：（1）连续型波形未用到这些参数
      （2）断续型波形用到interval_time 、 pulse_quantity
      （3）EMG波形用到total_time 、interval_time 、 time_of_stay_at_high 、 rising_time 、down_time
*/
struct Waveform_Thumbnail{
        uint32_t total_time;			//扫频波形的一组输出的总时长	(单位:ms  范围:0-)
		uint32_t interval_time;			//不连续波形组的间隔时长		(单位:ms  范围:0-)
		uint32_t pulse_quantity;		//一组刺激波形的脉冲数量		(单位:个  范围:0-)
		uint32_t time_of_stay_at_high;	//最高输出点的保持时间 			(单位:ms  范围:0-)
		uint32_t rising_time;			//爬坡时间		(单位:ms  范围:0-)
		uint32_t down_time;				//下坡时间		(单位:ms  范围:0-)
};


//四个通道的赋值结构体
//struct Waveform_Register Channel_Wave_Reg[4];
struct Waveform_Thumbnail Channel_Wave_Thumb[4]; //缩略图

//功能：处理接收到的数据
//实现：将接收到的电流值拆分
//      判断升降时间，如需升降，计算升降相关的数据：上升台阶数，同一台阶重复次数

//输入ma级别电流(float 类型) 可取出 unit 和 isel 分别的设置,本例程未用到，这一部分在上位机里处理，实现电流的伪无极调整
int setCurrentStep_33uA(float mA ,uint32_t *unit , uint32_t *isel)   
{
				uint16_t classCurrent = 0;     
				//按照unit分等级
				classCurrent = (uint16_t)((mA * 1000) / (33 * 255));  // class 就是unit  ,( 33 * unit * isel = ua) 计算结果unit应该为 ： 0 -- 8  	
				*unit = classCurrent;
				*isel = (uint16_t)((mA * 1000)/((classCurrent+1)*33));   
				return 0; 	
}

typedef struct EMS_Wave_Para{
	float   Set_CurrentArray_From_Para[33];   //用于自动填充电流大小的数组（幅值）
	int8_t   Current_Current_Gear;   //当前电流挡位
	uint32_t  set_step_current_int ; // 64 档 每一档的电流大小(转换为了int型)
	uint32_t  rising_timeQuotients;
	uint32_t  rising_timeRemainders;
	uint32_t  silent_addTime[3];  //需要给静默时间增加的时间
	uint32_t  down_timeQuotients;
	uint32_t  down_timeRemainders;
	uint32_t  time_of_stayQuotients;
	uint32_t  time_of_stayRemainders;
	uint32_t  unit_current_for_EMS[33];
	uint32_t  class_current_for_EMS[33];
}EMS_Wave_Para_Array;

EMS_Wave_Para_Array struct_EMS_Para[4];

//缓升缓降波形
//注意，最高点的电流值最低需要设置到3mA！！！ ， 升/降时间最少需要500ms，保持时间需要大于一个周期的时长
//只能设置正半周期脉宽 ，死区时间，最高点幅值 ，其他的自动设置！！！ 
//商和余数 Quotients and remainders 
uint8_t EMS_Wave_Para_Cal(uint8_t channel_num,struct Waveform_Thumbnail Wave_Thumb,struct_Wave_Reg Wave_Reg)
{
	//这里使用的是上位机下发的最大电流值，需要把他们中的每一档都拆成不同的单元电流和电流挡位
	struct_EMS_Para[channel_num].set_step_current_int = (uint32_t)((Wave_Reg.ADDR_WG_DRV_IN_WAVE_REG+1) * (Wave_Reg.ADDR_WG_DRV_ISEL_REG+1) * 33  / 33 ); //根据此值填充
	                                                                                                                     //Set_CurrentArray_From_Para[33]数组 
	                                                                                                                     //第一个33是单元电流，第二个33是缓升缓降的数组间隔（34-1）
	//填充数组，数组0-32升降，第33个数保持	
	for(int i=0;i<33;i++){
		struct_EMS_Para[channel_num].Set_CurrentArray_From_Para[i]= struct_EMS_Para[channel_num].set_step_current_int * (i+1) ;	
		setCurrentStep_33uA((float)(struct_EMS_Para[channel_num].Set_CurrentArray_From_Para[i]/1000) ,&struct_EMS_Para[channel_num].unit_current_for_EMS[i] , &struct_EMS_Para[channel_num].class_current_for_EMS[i]) ;  //取单元电流和电流挡位的值			
	}
	
	struct_EMS_Para[channel_num].rising_timeQuotients = (uint32_t)(Wave_Thumb.rising_time *1000*1000 /32/ (Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG));
	struct_EMS_Para[channel_num].rising_timeRemainders =  (uint32_t)(Wave_Thumb.rising_time *1000*1000 % (32*(Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG)));
	struct_EMS_Para[channel_num].silent_addTime[0] = (uint32_t)(struct_EMS_Para[channel_num].rising_timeRemainders  / struct_EMS_Para[channel_num].rising_timeQuotients / 32);
	Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG += struct_EMS_Para[channel_num].silent_addTime[0];
		
	struct_EMS_Para[channel_num].time_of_stayQuotients = (uint32_t)(Wave_Thumb.time_of_stay_at_high *1000*1000 / (Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG));
	struct_EMS_Para[channel_num].time_of_stayRemainders =  (uint32_t)(Wave_Thumb.time_of_stay_at_high *1000*1000 % (Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG));
	struct_EMS_Para[channel_num].silent_addTime[1] = (uint32_t)(struct_EMS_Para[channel_num].time_of_stayRemainders  / struct_EMS_Para[channel_num].time_of_stayQuotients );
	Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG += struct_EMS_Para[channel_num].silent_addTime[1];
		
	struct_EMS_Para[channel_num].down_timeQuotients = (uint32_t)(Wave_Thumb.down_time *1000*1000 /32/ (Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG));
	struct_EMS_Para[channel_num].down_timeRemainders =  (uint32_t)(Wave_Thumb.down_time *1000*1000 % (32*(Wave_Reg.ADDR_WG_DRV_HLF_WAVE_PRD_REG * 2 + Wave_Reg.ADDR_WG_DRV_REST_T_REG + Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG)));
	struct_EMS_Para[channel_num].silent_addTime[2] = (uint32_t)(struct_EMS_Para[channel_num].down_timeRemainders  / struct_EMS_Para[channel_num].down_timeQuotients / 32);
	Wave_Reg.ADDR_WG_DRV_ALT_SILENT_LIM_REG += struct_EMS_Para[channel_num].silent_addTime[2];
	//确定了各段的级数以及单级重复次数后，开启刺激波形
		
	return channel_num;
}


#endif 