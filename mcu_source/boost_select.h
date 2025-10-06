#ifndef BOOST_SELECT_H
#define BOOST_SELECT_H

#include <stdio.h>
#include "CMSDK_CM0.h"

extern uint8_t Boost_Voltage_Sel(uint8_t VOLTAGE_XV);

#define   VOLTAGE_11V  0x0
#define   VOLTAGE_15V  0x1
#define   VOLTAGE_26V  0x2
#define   VOLTAGE_45V  0x3
#define   VOLTAGE_55V  0x4

uint8_t Boost_Voltage_Sel(uint8_t VOLTAGE_XV)
{   
	
	switch (VOLTAGE_XV){
		case VOLTAGE_11V :
			CMSDK_ANAC->BOOST_CTRL =0x71013;//内部boost	
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
		
		case VOLTAGE_15V :
			CMSDK_ANAC->BOOST_CTRL =0x71113;//内部boost	
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
		case VOLTAGE_26V :
			CMSDK_ANAC->BOOST_CTRL =0x71213;//内部boost	
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
		case VOLTAGE_45V :
			CMSDK_ANAC->BOOST_CTRL =0xc1413;//
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
		case VOLTAGE_55V :
			CMSDK_ANAC->BOOST_CTRL =0xc1713;//内部boost	
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
		default:
			CMSDK_ANAC->BOOST_CTRL =0x71013;//内部boost	
			CMSDK_ANAC->PMU_CTRL = 0x10;
			break;
	}
	return VOLTAGE_XV;
}

#endif

 













