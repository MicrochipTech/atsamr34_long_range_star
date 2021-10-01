/**
* \file  demo_output.h
*
* \brief Demo output Interface.
*
* Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/

#ifndef _DEMO_OUPUT_H
#define _DEMO_OUTPUT_H

#define MAX_SEC_UCAST_PAYLOAD  218							
#define MAX_NSEC_UCAST_PAYLOAD  227							

#define MAX_SEC_BCAST_PAYLOAD  218							
#define MAX_NSEC_BCAST_PAYLOAD  227							

/*********************************************************************
* Function: void DemoOutput_Greeting( void )
*
* Overview: Prints demo greeting message
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_Greeting(void);

/*********************************************************************
* Function: void DemoOutput_Channel(uint8_t channel, uint8_t step)
*
* Overview: Prints demo channel
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_Channel(uint8_t channel, uint8_t step);

/*********************************************************************
* Function: void DemoOutput_Instruction( void )
*
* Overview: Prints demo instructions
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_Instruction(void);

/*********************************************************************
* Function: void DemoOutput_HandleMessage( void )
*
* Overview: Prints demo message type
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_HandleMessage(void);

/*********************************************************************
* Function: void DemoOutput_UpdateTxRx( uint8_t TxNum, uint8_t RxNum )
*
* Overview: Prints Tx and Rx Packet Count
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_UpdateTxRx(uint8_t TxNum, uint8_t RxNum);

/*********************************************************************
* Function: void DemoOutput_ChannelError( uint8_t channel )
*
* Overview: Outputs channel error message
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_ChannelError(uint8_t channel);

/*********************************************************************
* Function: void DemoOutput_UnitcastFail( void )
*
* Overview: Outputs Unicast fail status
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void DemoOutput_UnicastFail(void);

void DemoOutput_BroadcastFail(void);
/******************************************************************/
//STAR SUPPORT CODE
#if defined(PROTOCOL_STAR)
/*********************************************************************
 * Function: void STAR_DEMO_OPTIONS_MESSAGE( DeviceRole_t deviceRole )
 *
 * Overview: Displays available options for the DEMO kit in STAR Network
 *
 * PreCondition: None
 *
 * Input:  DeviceRole_t deviceRole- PAN_COORD or END_DEVICE
 *
 * Output: None
 *
 ********************************************************************/
void STAR_DEMO_OPTIONS_MESSAGE(DeviceRole_t deviceRole);

/*********************************************************************
 * Function: void Source_END_DEVICE_INFO( bool )
 *
 * Overview: This function is only used for STAR NETWORK
 *                  For a Packet_TX from EDx--> PAN CO --> EDy
 *                  EDy shows the unique address of EDx to confirm
 *                  it received a packet from EDx
 * PreCondition: None
 *
 * Input:  Pointer to Unique address of EDx 
 *
 * Output: None
 * 
 * Note : Only for Demo Purposes , App developers can avoid using if 
 *          sure of packet transmission
 *
 ********************************************************************/
void Source_END_DEVICE_INFO(uint8_t *);
#endif
/*********************************************************************
* Function: uint8_t ButtonPressed(void)
*
* Overview: Reads the Button state
*
* PreCondition: None
*
* Input:  None
*
* Output: Buttons State
*
********************************************************************/
uint8_t ButtonPressed(void);

/*********************************************************************
* Function: uint8_t Buttons_init(void)
*
* Overview: configures buttons
*
* PreCondition: None
*
* Input:  None
*
* Output: None
*
********************************************************************/
void Buttons_init(void);

/*********************************************************************
* Function: void demo_output_freezer_options(void)
*
* Overview: Display Freezer Options
*
* PreCondition: Timer Initialize , system Initialize
*
* Input:  None
*
* Output: None
*
********************************************************************/

void demo_output_freezer_options(void);

#endif

