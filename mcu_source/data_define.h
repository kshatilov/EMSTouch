//���ܣ������ĸ�ͨ���õ������飬ͨ�ñ�����
#ifndef DATA_DEFINE_H
#define DATA_DEFINE_H
#include <stdio.h>
#include "CMSDK_CM0.h"


//���ε�ַ��Ӧ�ĵ�����ֵ����

struct struct_wave_data{
	int wave_data[64];
};
struct struct_wave_data wave_data_array[4];

//CONFIG�Ĵ������ݵĽṹ�壬���������ļĴ����Ĳ���֮ǰ
//��Ҫ��ȷ��config������

uint8_t ADDR_WG_DRV_CONFIG_REG_SAVE =0x00;
typedef struct Wave_Config{
	uint8_t rest_en;      //����ʹ�� 
	uint8_t negtive_en;   //�����ܲ���ʹ��
	uint8_t silent_en;    //��Ĭʹ��
	uint8_t source_B_en;  //source_B ʹ��
	uint8_t continue_en;
	uint8_t multi_electrode_en;
}struct_Wave_Config;

struct_Wave_Config Config_Reg[4]; //���ڱ��洮�ڷ�����config���ݣ���Ҫ��ԭ16��������ת�����˽ṹ���У�


//1 ���沨�η������ļĴ������ݵĽṹ�壨1��
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


struct_Wave_Reg Channel_Reg_Init[4];   //��ʼ���ã���Ҫ��ǰ����ֵ
struct_Wave_Reg Channel_Reg[4]={0};        //���ղ�����ֵ



/*
2 ��������ͼʱ�����������
���ࣺ��1�������Ͳ���δ�õ���Щ����
      ��2�������Ͳ����õ�interval_time �� pulse_quantity
      ��3��EMG�����õ�total_time ��interval_time �� time_of_stay_at_high �� rising_time ��down_time
*/
struct Waveform_Thumbnail{
        uint32_t total_time;			//ɨƵ���ε�һ���������ʱ��	(��λ:ms  ��Χ:0-)
		uint32_t interval_time;			//������������ļ��ʱ��		(��λ:ms  ��Χ:0-)
		uint32_t pulse_quantity;		//һ��̼����ε���������		(��λ:��  ��Χ:0-)
		uint32_t time_of_stay_at_high;	//��������ı���ʱ�� 			(��λ:ms  ��Χ:0-)
		uint32_t rising_time;			//����ʱ��		(��λ:ms  ��Χ:0-)
		uint32_t down_time;				//����ʱ��		(��λ:ms  ��Χ:0-)
};


//�ĸ�ͨ���ĸ�ֵ�ṹ��
//struct Waveform_Register Channel_Wave_Reg[4];
struct Waveform_Thumbnail Channel_Wave_Thumb[4]; //����ͼ

//���ܣ�������յ�������
//ʵ�֣������յ��ĵ���ֵ���
//      �ж�����ʱ�䣬��������������������ص����ݣ�����̨������ͬһ̨���ظ�����

//����ma�������(float ����) ��ȡ�� unit �� isel �ֱ������,������δ�õ�����һ��������λ���ﴦ��ʵ�ֵ�����α�޼�����
int setCurrentStep_33uA(float mA ,uint32_t *unit , uint32_t *isel)   
{
				uint16_t classCurrent = 0;     
				//����unit�ֵȼ�
				classCurrent = (uint16_t)((mA * 1000) / (33 * 255));  // class ����unit  ,( 33 * unit * isel = ua) ������unitӦ��Ϊ �� 0 -- 8  	
				*unit = classCurrent;
				*isel = (uint16_t)((mA * 1000)/((classCurrent+1)*33));   
				return 0; 	
}

typedef struct EMS_Wave_Para{
	float   Set_CurrentArray_From_Para[33];   //�����Զ���������С�����飨��ֵ��
	int8_t   Current_Current_Gear;   //��ǰ������λ
	uint32_t  set_step_current_int ; // 64 �� ÿһ���ĵ�����С(ת��Ϊ��int��)
	uint32_t  rising_timeQuotients;
	uint32_t  rising_timeRemainders;
	uint32_t  silent_addTime[3];  //��Ҫ����Ĭʱ�����ӵ�ʱ��
	uint32_t  down_timeQuotients;
	uint32_t  down_timeRemainders;
	uint32_t  time_of_stayQuotients;
	uint32_t  time_of_stayRemainders;
	uint32_t  unit_current_for_EMS[33];
	uint32_t  class_current_for_EMS[33];
}EMS_Wave_Para_Array;

EMS_Wave_Para_Array struct_EMS_Para[4];

//������������
//ע�⣬��ߵ�ĵ���ֵ�����Ҫ���õ�3mA������ �� ��/��ʱ��������Ҫ500ms������ʱ����Ҫ����һ�����ڵ�ʱ��
//ֻ������������������ ������ʱ�䣬��ߵ��ֵ ���������Զ����ã����� 
//�̺����� Quotients and remainders 
uint8_t EMS_Wave_Para_Cal(uint8_t channel_num,struct Waveform_Thumbnail Wave_Thumb,struct_Wave_Reg Wave_Reg)
{
	//����ʹ�õ�����λ���·���������ֵ����Ҫ�������е�ÿһ������ɲ�ͬ�ĵ�Ԫ�����͵�����λ
	struct_EMS_Para[channel_num].set_step_current_int = (uint32_t)((Wave_Reg.ADDR_WG_DRV_IN_WAVE_REG+1) * (Wave_Reg.ADDR_WG_DRV_ISEL_REG+1) * 33  / 33 ); //���ݴ�ֵ���
	                                                                                                                     //Set_CurrentArray_From_Para[33]���� 
	                                                                                                                     //��һ��33�ǵ�Ԫ�������ڶ���33�ǻ�����������������34-1��
	//������飬����0-32��������33��������	
	for(int i=0;i<33;i++){
		struct_EMS_Para[channel_num].Set_CurrentArray_From_Para[i]= struct_EMS_Para[channel_num].set_step_current_int * (i+1) ;	
		setCurrentStep_33uA((float)(struct_EMS_Para[channel_num].Set_CurrentArray_From_Para[i]/1000) ,&struct_EMS_Para[channel_num].unit_current_for_EMS[i] , &struct_EMS_Para[channel_num].class_current_for_EMS[i]) ;  //ȡ��Ԫ�����͵�����λ��ֵ			
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
	//ȷ���˸��εļ����Լ������ظ������󣬿����̼�����
		
	return channel_num;
}


#endif 