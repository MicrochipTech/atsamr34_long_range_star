/**
* \file  star_demo.c
*
* \brief Demo Application for MiWi P2P Implementation
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

/************************ HEADERS ****************************************/
#include "miwi_api.h"
#include "miwi_p2p_star.h"
#include "task.h"
#include "star_demo.h"
#include "mimem.h"
#include "asf.h"
#if defined(ENABLE_SLEEP_FEATURE)
#include "sleep_mgr.h"
#endif
#if defined (ENABLE_CONSOLE)
#include "sio2host.h"
#endif

#include "phy.h"
#include "sw_timer.h"

#if defined(PROTOCOL_P2P) || defined(PROTOCOL_STAR)

uint8_t i;
uint8_t TxNum = 0;
uint8_t RxNum = 0;
bool chk_sel_status = true;  // Check _select_Status
bool update_ed;
uint8_t select_ed;
uint8_t msghandledemo = 0;
/**************************/
//STAR SUPPORT CODE
#if defined(PROTOCOL_STAR)
bool display_connections;
MIWI_TICK t1 , t2;
//#if defined(ENABLE_SLEEP_FEATURE)
extern volatile bool LinkStatus;
extern volatile bool SendData;
//#endif
#endif
/**************************/
	
#if defined(ENABLE_SLEEP_FEATURE)
volatile uint8_t send_data = 0;	// Flag used to put a sleeping device(RFD) in sleep - wakeup - transmit cycle
#endif // #if defined(ENABLE_SLEEP_FEATURE)

/* Payload size for data sent out using periodic transmission */
#define PAYLOAD_SIZE 40		// stay below MAX_SEC_UCAST_PAYLOAD
/* Payload for data sent out using periodic transmission */
#define PAYLOAD      "HelloWorldHelloWorldHelloWorldHelloWorld"

#ifdef DUTY_CYCLING
/* Duty cycling percentage - range : 1 to 99 */
#define dutyCyclePercentage				1
/* Stores the application payload length of each data for duty cycling calculation in confirmation callback */
uint8_t appDataPhyLen ;
/* Remaining duty cycling duration to wait before sending next app data */
uint32_t remainingDutyCycleDuration ;
#endif

#define BROADCAST	0
#define UNICAST		1

#ifdef DUTY_CYCLING
// Callback when the timer ran for duty cycling expires
void dutyCyclingCallback(void)
{
	/* This can be used to indicate out of duty cycling mode, like switch on LED. */	
}
#endif // #ifdef DUTY_CYCLING

#ifdef PERIODIC_TX
// Periodic Transmission callback
void periodicTxCallback(void)
{
	p2p_demo_send_data(UNICAST, 0, PAYLOAD, PAYLOAD_SIZE) ;
	if(TxNum < 100000)
	{
		printf("Periodic transmission timer reloaded to %d ms\r\n", DATA_SENDING_INTERVAL_MS) ;
		SwTimerStart (PeriodicTxTimerId, MS_TO_US(DATA_SENDING_INTERVAL_MS) , 0/*SW_TIMEOUT_RELATIVE*/, (void *)periodicTxCallback, NULL) ;
	}
	else
	{
		SwTimerStop(PeriodicTxTimerId) ;
		TxNum = 0 ;
	}
}
#endif //#ifdef PERIODIC_UPLINK

// Transmission time-out callback
void TxToutCallback(void)
{
#if defined(ENABLE_SLEEP_FEATURE)
	send_data = 0;
#endif // #if defined(ENABLE_SLEEP_FEATURE)
	/* That bring the node back to continuous transaction cycle */
	PHY_DataConf(TRANSACTION_EXPIRED);
	SwTimerStop(TxTimerId);
}

// Transmission callback
void dataConfcb(uint8_t handle, miwi_status_t status, uint8_t* msgPointer)
{
#if defined (ENABLE_CONSOLE)
	printf("\nData Confirm: Handle: %d status:%d \r\n", handle, status);
#endif // #if defined (ENABLE_CONSOLE)
#if defined(ENABLE_SLEEP_FEATURE)
	send_data = 0;
#endif // #if defined(ENABLE_SLEEP_FEATURE)
#if defined(PROTOCOL_STAR)
    SendData = false;
#endif
	/* Stop transmission timeout timer */
	SwTimerStop(TxTimerId);
	/* Free payload buffer allocated */
	MiMem_Free(msgPointer);
	
#ifdef DUTY_CYCLING
	uint32_t dutyCyclingDurationMilliSec ;
	uint8_t txCount ;
	uint8_t dr ;
	RADIO_GetAttr(SPREADING_FACTOR, &dr) ;	
	// calculate duty cycling duration using the payload length and transmission status
	uint16_t timeOnAirMilliSec = calculate_ToA(appDataPhyLen) ;
	#if defined (ENABLE_CONSOLE)
	printf("Time-on air calculated : %d ms to transmit %d bytes in SF%d\r\n", timeOnAirMilliSec, appDataPhyLen, dr) ;
	#endif
	if (NO_ACK == status)
	{
		/* If the packet was not acknowledged, then the hardware would retry for DATA_RETRY_ATTEMPT */
		txCount = DATA_RETRY_ATTEMPT + 1 ;
	}
	else if (SUCCESS == status)
	{
		/* If the packet was acknowledged and success, frame was sent only once */
		txCount = 1 ;		
	}
	else
	{
		/* If status is CHANNEL_ACCESS_FAILURE, FAILURE etc..., then there is zero transmission */
		txCount = 0 ;
	}
	dutyCyclingDurationMilliSec = ((100 - dutyCyclePercentage) * (timeOnAirMilliSec * txCount)) ;
	// start the duty cycling timer
	SwTimerStart (DutyCyclingTimerId, MS_TO_US(dutyCyclingDurationMilliSec) , 0/*SW_TIMEOUT_RELATIVE*/, (void *)dutyCyclingCallback, NULL) ;
	#if defined (ENABLE_CONSOLE)
	printf("Send next data after: %ld ms\r\n", dutyCyclingDurationMilliSec) ;
	#endif
#endif // #ifdef DUTY_CYCLING
}

void run_star_demo(void)
{
#if defined(PROTOCOL_STAR)
  t1.Val = MiWi_TickGet ();
  display_connections = true;
  while(display_connections)
  {
	  P2PTasks();
	  SYSTEM_RunTasks();
#endif	  
#if defined(ENABLE_SLEEP_FEATURE)
#if defined(PROTOCOL_STAR)
	if ((role == END_DEVICE) && Total_Connections())
#else
	if (Total_Connections())
#endif
	{
		#if defined(PROTOCOL_STAR)
		/* At least device is connected to one peer device */
		if(!(P2PStatus.bits.DataRequesting || P2PStatus.bits.RxHasUserData || send_data || LinkStatus))
		#else
		if(!(P2PStatus.bits.DataRequesting || P2PStatus.bits.RxHasUserData || send_data))
		#endif
		{
			/* Put the transceiver into sleep */
			MiApp_TransceiverPowerState(POWER_STATE_SLEEP);
#if defined (ENABLE_CONSOLE)
			printf("\r\nDevice is sleeping");
#if defined(PROTOCOL_STAR)
#if defined (ENABLE_LCD)
            LCD_Erase();
            snprintf(LCDText,sizeof(LCDText), "Device is sleeping");
            LCD_Update();
#endif
#endif
			/* Disable UART */
			sio2host_disable();
#endif // #if defined (ENABLE_CONSOLE)
			// Turned OFF RF Switch
			struct port_config pin_conf;
			port_get_config_defaults(&pin_conf);
			pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
			port_pin_set_config(RF_SWITCH_PIN, &pin_conf);
			port_pin_set_output_level(RF_SWITCH_PIN, RF_SWITCH_INACTIVE);

			uint32_t sleep_interval = RFD_WAKEUP_INTERVAL - 2 ;	// default sleep interval
			uint8_t power_state_mode = POWER_STATE_WAKEUP_DR ;	// selected power state mode
			
#ifdef DUTY_CYCLING
			// adjust the sleep_interval to fit with duty cycling calculation
			uint32_t dutyCyclingDurationMilliSec ;
			uint16_t timeOnAirMilliSec ;
			if (power_state_mode == POWER_STATE_WAKEUP_DR)
			{
				appDataPhyLen = 1 ;	// data request frame size from sleeping device in bytes 
			}
			else
			{
				appDataPhyLen = PAYLOAD_SIZE ;
			}
			timeOnAirMilliSec = calculate_ToA(appDataPhyLen) ;
			dutyCyclingDurationMilliSec = (100 - dutyCyclePercentage) * (timeOnAirMilliSec) ;
			if (sleep_interval*1000 < dutyCyclingDurationMilliSec)
			{
				sleep_interval = dutyCyclingDurationMilliSec/1000 ;
			}
#endif
			// stop all existing timers
			/* Put the MCU into sleep */
			sm_sleep(sleep_interval) ;

#if defined (ENABLE_CONSOLE)
			/* Enable UART */
			sio2host_enable();
			printf("\r\nDevice WokeUp after %ld sec\r\n", sleep_interval) ;
//#if defined(PROTOCOL_STAR)
//#if defined (ENABLE_LCD)
//            LCD_Erase();
//            snprintf(LCDText,sizeof(LCDText), "Device WokeUp after %ld sec\r\n",sleep_interval);
//            LCD_Update();
//#endif
//#endif
#endif // #if defined (ENABLE_CONSOLE)
			/* Wakeup the transceiver and send data request */
			MiApp_TransceiverPowerState(power_state_mode);
			if (power_state_mode == POWER_STATE_WAKEUP)
				p2p_demo_send_data(UNICAST, 0, PAYLOAD, PAYLOAD_SIZE) ;
		}
	}
#endif // #if defined(ENABLE_SLEEP_FEATURE)
	{
		/*******************************************************************/
		// If no packet received, now we can check if we want to send out
		// any information.
		// Function ButtonPressed will return if any of the two buttons
		// has been pushed.
		/*******************************************************************/
		
		/* Read the button */
		uint8_t PressedButton = ButtonPressed();
#if defined(PROTOCOL_STAR)
		if ( PressedButton == 1 || PressedButton == 2)
		{
#else
		switch( PressedButton )
		{
			case 1:
			{
#endif
#if defined(PROTOCOL_STAR)
			if (role == PAN_COORD)
			{
#endif
				/*******************************************************************/
				// Button 1 pressed - prepare a broadcast message
				/*******************************************************************/
#ifdef DUTY_CYCLING
				remainingDutyCycleDuration = SwTimerReadValue(DutyCyclingTimerId) ;
				#if defined (ENABLE_CONSOLE)
				printf("\r\nRemaining duty cycling duration to wait before sending next app data: %ld us\r\n", remainingDutyCycleDuration) ;
				#endif
				if (remainingDutyCycleDuration)
				{
					#if defined (ENABLE_CONSOLE)
					printf("NO_FREE_CH\r\n") ;
					#endif
					return ;
				}
#endif // #ifdef DUTY_CYCLING
				uint8_t* dataPtr = NULL;
				uint8_t dataLen = 0;
				uint16_t broadcastAddress = 0xFFFF;
#if defined(ENABLE_SECURITY)
				/* Allocate buffer for secured payload */
				dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_SEC_BCAST_PAYLOAD)); 
				if (NULL == dataPtr)
					return;
				for(i = 0; i < MAX_SEC_BCAST_PAYLOAD; i++) 
#else
				/* Allocate buffer for non-secured payload */
				dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_NSEC_BCAST_PAYLOAD)); 
				if (NULL == dataPtr)
					return;
				for(i = 0; i < MAX_NSEC_BCAST_PAYLOAD; i++) 
#endif // #if defined(ENABLE_SECURITY)
				{
					// Fill TX buffer User DATA
					dataPtr[dataLen++] = 0x42;	// 'B'
				}
#if defined(ENABLE_SLEEP_FEATURE)
				send_data = 1;
#endif // #if defined(ENABLE_SLEEP_FEATURE)

#ifdef DUTY_CYCLING
				appDataPhyLen = dataLen ;
#endif // #ifdef DUTY_CYCLING
				
					/* Broadcast the message */
					if(MiApp_SendData(SHORT_ADDR_LEN, (uint8_t *)&broadcastAddress, dataLen, dataPtr, ++msghandledemo, true, dataConfcb)== false)
					{
#if defined(ENABLE_SLEEP_FEATURE)
						PHY_DataConf(FAILURE);
#endif // #if defined(ENABLE_SLEEP_FEATURE)
						DemoOutput_BroadcastFail();
					}
					else
					{
						/* Start timer for transmission timeout */
						//SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL);
					}
#if defined(PROTOCOL_STAR)
			 }
#else
					/* Update display/console */
					DemoOutput_UpdateTxRx(++TxNum, RxNum);
					DemoOutput_Instruction();
#endif
#if !defined(PROTOCOL_STAR)
				}
				break;
				case 2:
				{
#endif
#if defined(PROTOCOL_STAR)
				else
				{
#endif
					/*******************************************************************/
					// Button 2 pressed - prepare a unicast message
					/*******************************************************************/
					chk_sel_status = true;
					select_ed = 0;
					update_ed = true;
					while(update_ed == true)
					{
						// Peer Device Info

#if defined(PROTOCOL_STAR)
#if defined (ENABLE_LCD)
#if !defined(ENABLE_SLEEP_FEATURE)
						LCD_Erase();
						if (myConnectionIndex_in_PanCo  == select_ed)
						{        /* if END_device displays itself , "me" is added in display to denote itself */
							snprintf(LCDText, sizeof(LCDText), "SW0:%02d-%02x%02x%02x-me \nBUTTON1: Change node", END_DEVICES_Short_Address[select_ed].connection_slot,END_DEVICES_Short_Address[select_ed].Address[0],
							END_DEVICES_Short_Address[select_ed].Address[1],END_DEVICES_Short_Address[select_ed].Address[2] );
							LCD_Update();
						}
						else
						{
							snprintf(LCDText, sizeof(LCDText), "SW0:%02d-%02x%02x%02x \nBUTTON1: Change node", END_DEVICES_Short_Address[select_ed].connection_slot,END_DEVICES_Short_Address[select_ed].Address[0],
							END_DEVICES_Short_Address[select_ed].Address[1],END_DEVICES_Short_Address[select_ed].Address[2] );
							LCD_Update();
						}
						LCD_Update();
#endif
#endif
#else
#if defined (ENABLE_LCD)
						LCD_Erase();
						snprintf(LCDText, sizeof(LCDText),(char*)"SW:%02d-%02x%02x%02x \nBUTTON1: Change node",select_ed,ConnectionTable[select_ed].Address[0],
						ConnectionTable[select_ed].Address[1],ConnectionTable[select_ed].Address[2]);
						LCD_Update();
#endif // #if defined (ENABLE_LCD)
#endif
					// Display another Peer Device Address
						chk_sel_status = true;

						while(chk_sel_status)
						{
							uint8_t switch_val = ButtonPressed();
							
							// While waiting in TX , RX will process if any message was available
							if(switch_val == 1)
							{
#ifdef DUTY_CYCLING
								remainingDutyCycleDuration = SwTimerReadValue(DutyCyclingTimerId) ;
								#if defined (ENABLE_CONSOLE)
								printf("\r\nRemaining duty cycling duration to wait before sending next app data: %ld us\r\n", remainingDutyCycleDuration) ;
								#endif
								if (remainingDutyCycleDuration)
								{
									#if defined (ENABLE_CONSOLE)
									printf("NO_FREE_CH\r\n") ;
									#endif
									return ;
								}
#endif // #ifdef DUTY_CYCLING
								uint8_t* dataPtr = NULL;
								uint8_t dataLen = 0;
								update_ed = false;
								chk_sel_status = false;
#if defined(ENABLE_SECURITY)
								/* Allocate buffer for secured payload */
								dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_SEC_UCAST_PAYLOAD));
								if (NULL == dataPtr)
									return;
								for(i = 0; i < MAX_SEC_UCAST_PAYLOAD; i++)        
#else
									/* Allocate buffer for non-secured payload */
							        dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_NSEC_UCAST_PAYLOAD));    
								if (NULL == dataPtr)
							        return;
								for(i = 0; i < MAX_NSEC_UCAST_PAYLOAD; i++)        
#endif // #if defined(ENABLE_SECURITY)
								{
									// Fill TX buffer User DATA
									dataPtr[dataLen++] = 0x55;	// 'U'
								}
#if defined(ENABLE_SLEEP_FEATURE)
								send_data = 1;
#endif // #if defined(ENABLE_SLEEP_FEATURE)

#ifdef DUTY_CYCLING
								appDataPhyLen = dataLen ;
#endif // #ifdef DUTY_CYCLING

#if defined(PROTOCOL_STAR)
					                 if (myConnectionIndex_in_PanCo == select_ed)
					                    {
					                        /* IF on the demo , a END_Device displays its own Connection Detail
					                           unicast data packet to just PAN COR , No forwarding */
					                         if (MiApp_SendData(LONG_ADDR_LEN, ConnectionTable[0].Address, dataLen, dataPtr, ++msghandledemo, true, dataConfcb)== false)
											 {
							#if defined(ENABLE_SLEEP_FEATURE)
												 PHY_DataConf(FAILURE);
							#endif // #if defined(ENABLE_SLEEP_FEATURE)
												 --msghandledemo;
							                     printf("\nPrevious Transaction is ongoing\n\r");
												 //DemoOutput_UnicastFail();
											 }
											 else
											 {
											 	// Successful Transmission
											 	TxNum++;
											 	/* Start timer for transmission timeout */
											 	//SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL);
											 }
											

					                    }
						             else
						             {
						                 /* Data can be sent at a time from one END_DEVICE_TO_ANOTHER
						                    Edx --> Pan CO --> EDy
						                    To forward a Packet from one ED to another ED , address should be specified with length as 3
						                    and address as end dest device short address (3 bytes)
						                 */
						                 if(MiApp_SendData(3, END_DEVICES_Short_Address[select_ed].Address,dataLen, dataPtr, ++msghandledemo, true, dataConfcb)==false)

											 {
										#if defined(ENABLE_SLEEP_FEATURE)
												 PHY_DataConf(FAILURE);
										#endif // #if defined(ENABLE_SLEEP_FEATURE)
										         --msghandledemo;
										         printf("\nPrevious Transaction is ongoing\n\r");
												 //DemoOutput_UnicastFail();
											 }
											 else
											 {
												  
												 // Successful Transmission
												 TxNum++;
												 /* Start timer for transmission timeout */
												// SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL);
											 }
						             }			
#else
							
								/* Unicast the message to select_ed node */
								if (MiApp_SendData(LONG_ADDR_LEN, ConnectionTable[select_ed].Address, dataLen, dataPtr, ++msghandledemo, 1, dataConfcb) == false)
								{
											/* That bring the node back to continuous transaction cycle */
#if defined(ENABLE_SLEEP_FEATURE)
									PHY_DataConf(FAILURE);
#endif // #if defined(ENABLE_SLEEP_FEATURE)
#if ! defined(PROTOCOL_STAR)
									DemoOutput_UnicastFail();
#endif
								}
								else
								{
									
									// Successful Transmission
									TxNum++;
									/* Start timer for transmission timeout */
									//SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL);
								}
								
								// Update the display/console
								DemoOutput_UpdateTxRx(TxNum, RxNum);
								DemoOutput_Instruction();
								
#endif
								break;
							} // if(switch_val == 1)
							else if(switch_val == 2)
							{
#if defined(PROTOCOL_STAR)
								if (select_ed > end_nodes-1)  /* Reset Peer Device Info */
								{
								 /* If end of Peer Device Info reset the count */
								 select_ed = 0;
								}
#else
								if (select_ed > conn_size-2)
								{
									// Last Peer Device
									select_ed = 0;
								}
#endif
								else
								{
									// Update the Display
									select_ed = select_ed+1;
								}
								chk_sel_status = false;
							} // else if(switch_val == 2)
							/* While waiting in TX , RX will process if any message was available */
							P2PTasks ();
							SYSTEM_RunTasks();
						} // while(chk_sel_status), end of Peer Device selection
					} // while(update_ed == true), end of Display
				}
#if ! defined(PROTOCOL_STAR)
				break;
				default:
				break;
#endif
			}
#if defined(PROTOCOL_STAR)
        t2.Val = MiWi_TickGet ();
        /* Display the no of End devices in Network */
        if( MiWi_TickGetDiff(t2, t1) > (60 * ONE_SECOND))
        {
			if((role == PAN_COORD))
			{
	        #if defined (ENABLE_LCD)
	        LCD_Erase();
			snprintf(LCDText,sizeof(LCDText), "Role:%02d",role);
			LCD_Update();
	        snprintf(LCDText, sizeof(LCDText), "\nEND_DEVICES :%02d",conn_size);
	        LCD_Update();
	        #endif
	        delay_ms(500);
	        display_connections = false;
	        STAR_DEMO_OPTIONS_MESSAGE (role);
			}
			//else if(role == END_DEVICE)
			//{
			//#if defined (ENABLE_LCD)
		    //LCD_Erase();
		    //snprintf(LCDText,sizeof(LCDText), "Role:%02d",role);
		    //LCD_Update();
			//#endif
			//delay_ms(500);
			//display_connections = false;
			//DemoOutput_Instruction();
			//}
		}
#endif
	}
  }
 #if defined(PROTOCOL_STAR)
	//P2PTasks ();
	//SYSTEM_RunTasks();
 #endif
}

void ReceivedDataIndication (RECEIVED_MESSAGE *ind)
{
    /*******************************************************************/
    // If a packet has been received, handle the information available
    // in rxMessage.
    /*******************************************************************/
	if ((myPANID.v[1] == ind->SourcePANID.v[1]) && (myPANID.v[0] == ind->SourcePANID.v[0]))
	{
#if defined(ENABLE_CONSOLE)
		DemoOutput_HandleMessage();
#endif
		/* Delay for Showing the contents on the display before showing instructions */
		delay_ms(500);
		
		/* Update the TX AND RX Counts on the display */
		DemoOutput_UpdateTxRx(TxNum, ++RxNum);
#if !defined(ENABLE_SLEEP_FEATURE)		
		// Toggle LED2 to indicate receiving a packet.
		LED_Toggle(LED0);
#endif

#if ! defined(PROTOCOL_STAR)
		DemoOutput_Instruction();
#endif
	}
	else
	{
		printf("Packet received from other Source PAN ID : 0x%x%x\r\n",ind->SourcePANID.v[1],ind->SourcePANID.v[0]);
	}
/****************************************************************************************/	
//STAR SUPPORT CODE
#if defined(PROTOCOL_STAR)
		

	    /* Display the Source End Device Info on reception msg, Do not display if it is
       a PAN CO or if the message received was a broadcast packet */
    if ((role == END_DEVICE) && !rxMessage.flags.bits.broadcast)
    {
        Source_END_DEVICE_INFO(rxMessage.Payload);
    }
    /* Display the Instructions message */
    STAR_DEMO_OPTIONS_MESSAGE (role);
#endif
/*****************************************************************************************/
}

// uint8_t msgType - BROADCAST or UNICAST
// uint8_t targetAddr - target address for unicast message
// uint8_t* data - data buffer
// uint8_t len - data length
void p2p_demo_send_data(uint8_t msgType, uint8_t targetAddr, uint8_t *data, uint8_t len)
{
#ifdef DUTY_CYCLING
	remainingDutyCycleDuration = SwTimerReadValue(DutyCyclingTimerId) ;
	#if defined (ENABLE_CONSOLE)
	printf("\r\nRemaining duty cycling duration to wait before sending next app data: %ld us\r\n", remainingDutyCycleDuration) ;
	#endif
	if (remainingDutyCycleDuration)
	{
		#if defined (ENABLE_CONSOLE)
		printf("NO_FREE_CH\r\n") ;
		#endif
		return ;
	}
#endif // #ifdef DUTY_CYCLING
	
	uint8_t* dataPtr = NULL ;
	uint8_t dataLen = 0 ;
	uint16_t broadcastAddress = 0xFFFF ;
	bool sendData_status ;	

	dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(len)) ;
	if (NULL == dataPtr)
		return ;
	
	for (i = 0; i < len; i++)
	{
		dataPtr[dataLen++] = data[i] ;
	}
#if defined(ENABLE_SLEEP_FEATURE)
	// to not enter to the sleep loop until data has been sent
	send_data = 1;
#endif // #if defined(ENABLE_SLEEP_FEATURE)

#if defined (ENABLE_CONSOLE)
	if (msgType == UNICAST)
	{
		printf("\r\nSending unicast message to %02d-%02x%02x%02x\r\n", targetAddr, ConnectionTable[targetAddr].Address[0], ConnectionTable[targetAddr].Address[1], ConnectionTable[targetAddr].Address[2]) ;		
	}
	else
	{
		printf("\r\nSending broadcast message\r\n") ;
	}
#endif

#ifdef DUTY_CYCLING
	appDataPhyLen = dataLen ;
#endif // #ifdef DUTY_CYCLING

	if (msgType == UNICAST)
	{
		// unicast message
		sendData_status = MiApp_SendData(LONG_ADDR_LEN, ConnectionTable[targetAddr].Address, dataLen, dataPtr, ++msghandledemo, 1, dataConfcb) ;
	}
	else
	{
		// broadcast message
		sendData_status = MiApp_SendData(SHORT_ADDR_LEN, (uint8_t *)&broadcastAddress, dataLen, dataPtr, ++msghandledemo, true, dataConfcb) ;
	}

	if (sendData_status == false)
	{
#if defined(ENABLE_SLEEP_FEATURE)
		PHY_DataConf(FAILURE);
#endif // #if defined(ENABLE_SLEEP_FEATURE)
		if (msgType == UNICAST)
			DemoOutput_UnicastFail();
		else
			DemoOutput_BroadcastFail() ;
	}
	else
	{
		// Successful Transmission
		TxNum++ ;
		SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL) ;
	}
	// Update the LCD
	DemoOutput_UpdateTxRx(TxNum, RxNum);
#if !defined(PROTOCOL_STAR)
	DemoOutput_Instruction();
#endif
	printf("Tx Messages: %d - Rx Messages: %d\r\n", TxNum, RxNum) ;	
}

// Unicast max. payload size to parent
void p2p_demo_unicast_to_parent(void)
{
#ifdef DUTY_CYCLING
	remainingDutyCycleDuration = SwTimerReadValue(DutyCyclingTimerId) ;
	#if defined (ENABLE_CONSOLE)
	printf("\r\nRemaining duty cycling duration to wait before sending next app data: %ld us\r\n", remainingDutyCycleDuration) ;
	#endif
	if (remainingDutyCycleDuration)
	{
		#if defined (ENABLE_CONSOLE)
		printf("NO_FREE_CH\r\n") ;
		#endif
		return ;
	}
#endif // #ifdef DUTY_CYCLING
	
	uint8_t* dataPtr = NULL;
	uint8_t dataLen = 0;
	update_ed = false;
	chk_sel_status = false;
#if defined(ENABLE_SECURITY)
	dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_SEC_UCAST_PAYLOAD));
	if (NULL == dataPtr)
		return;
	for(i = 0; i < MAX_SEC_UCAST_PAYLOAD; i++)
#else
	dataPtr = MiMem_Alloc(CALC_SEC_PAYLOAD_SIZE(MAX_NSEC_UCAST_PAYLOAD));
	if (NULL == dataPtr)
		return;
	for(i = 0; i < MAX_NSEC_UCAST_PAYLOAD; i++)
#endif
	{
		// Tx Buffer User Data
		dataPtr[dataLen++] = 0x55;
	}
#if defined(ENABLE_SLEEP_FEATURE)
	// to not enter to the sleep loop until data has been sent
	send_data = 1;
#endif // #if defined(ENABLE_SLEEP_FEATURE)

	select_ed = 0 ;    // parent addr: ConnectionTable[0].Address
	printf("\r\nTry to unicast to %02d-%02x%02x%02x\r\n", select_ed, ConnectionTable[select_ed].Address[0], ConnectionTable[select_ed].Address[1], ConnectionTable[select_ed].Address[2]) ;

#ifdef DUTY_CYCLING
	appDataPhyLen = dataLen ;
#endif // #ifdef DUTY_CYCLING

	if( MiApp_SendData(LONG_ADDR_LEN, ConnectionTable[select_ed].Address, dataLen, dataPtr, ++msghandledemo, 1, dataConfcb) == false)
	{
#if defined(ENABLE_SLEEP_FEATURE)
		PHY_DataConf(FAILURE);
#endif // #if defined(ENABLE_SLEEP_FEATURE)
       --msghandledemo;
#if ! defined(PROTOCOL_STAR)
		DemoOutput_UnicastFail();
#endif
	}
	else
	{
		// Successful Transmission
		TxNum++;
		SwTimerStart (TxTimerId, MS_TO_US(5000), 0/*SW_TIMEOUT_RELATIVE*/, (void *)TxToutCallback, NULL);
	}
	// Update the LCD
	DemoOutput_UpdateTxRx(TxNum, RxNum);
	DemoOutput_Instruction();
	printf("Tx Messages: %d - Rx Messages: %d\r\n", TxNum, RxNum) ;	
}

#endif // #if defined(PROTOCOL_P2P)