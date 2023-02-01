/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       can_receive.c/h
  * @brief      there is CAN interrupt function  to receive motor data,
  *             and CAN send function to send motor current to control motor.
  *             ������CAN�жϽ��պ��������յ������,CAN���ͺ������͵���������Ƶ��.
  * @note       
  * @history
  */

#include "CAN_receive.h"
#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx.h"
#include "chassis_task.h" 

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern chassis_move_t chassis_move;	

#define get_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (uint16_t)((data)[2] << 8 | (data)[3]);      \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
    }
	

static motor_measure_t motor_chassis[4]; //0 1 2 3
static motor_measure_t Forward_L,Forward_R,Back_R,Back_L;  //0 1 2 3
			
cap_measure_t get_cap;

static CAN_TxHeaderTypeDef  chassis_tx_message;
static uint8_t              chassis_can_send_data[8];
		
static CAN_TxHeaderTypeDef  rudder_tx_message;
static uint8_t              rudder_can_send_data[8];

static CAN_TxHeaderTypeDef  capid_tx_message;
static uint8_t              capid_can_send_data[8];
		
		
/**
  * @brief          hal��CAN�ص�����,���յ������
	* @author         XQL
  * @param[in]      hcan:CAN���ָ��
  * @retval         none
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
	if(hcan==&hcan1)
	{
    switch (rx_header.StdId)
    {
			  case CAN_3508_M1_ID:
        case CAN_3508_M2_ID:
        case CAN_3508_M3_ID:
        case CAN_3508_M4_ID:
        {
            static uint8_t i = 0;
           
            i = rx_header.StdId - CAN_3508_M1_ID;
            get_motor_measure(&motor_chassis[i], rx_data);
//           
            break;
        }
			  case CAN_GIMBAL_CALL_BACK_ID:
				{
					chassis_move.gimbal_data.relative_angle=(fp32)(rx_data[0] << 8 | rx_data[1])/10000.f;  //��̨��ԽǶ�
					break;
				}
				case CAN_CAPID:
				{
					uint16_t*pPowerdata=(uint16_t*)rx_data;
					get_cap.power=(fp32)get_cap.invot*get_cap.current; //�趨����
					get_cap.invot =   (fp32)pPowerdata[0]/100.f;       //�����ѹ       
					get_cap.capvot = (fp32)pPowerdata[1]/100.f;        //���ݵ�ѹ
					get_cap.current = (fp32)pPowerdata[2]/100.f;       //�������
					get_cap.power = (fp32)pPowerdata[3]/100.f;         //��ǰ����
					break;
				}
		
			
		}
		
	}
	else if(hcan==&hcan2)
	{
		switch (rx_header.StdId)
    {
					case CAN_Forward_L_ID:
			{
					get_motor_measure(&Forward_L, rx_data);
//            detect_hook(CHASSIS_MOTOR1_TOE + i);
					break;
			}					
			case CAN_Forward_R_ID:
			{
					get_motor_measure(&Forward_R, rx_data);
//            detect_hook(CHASSIS_MOTOR1_TOE + i);
					break;
			}
			case CAN_BACK_L_ID:
			{
					get_motor_measure(&Back_L, rx_data);
//            detect_hook(CHASSIS_MOTOR1_TOE + i);
					break;
			}					
			case CAN_BACK_R_ID:
			{
					get_motor_measure(&Back_R, rx_data);
//            detect_hook(CHASSIS_MOTOR1_TOE + i);
					break;
			}
        default:
        {
            break;
        }
     }
   }

}


/**
  * @brief          ���Ͷ������Ƶ���(0x205,0x206,0x207,0x208)
  * @author         XQL
  * @param[in]      forward_L: (0x205) 6020������Ƶ���, ��Χ [-30000,30000]
  * @param[in]      forward_R: (0x206) 6020������Ƶ���, ��Χ [-30000,30000]
  * @param[in]      back_L: (0x207) 6020������Ƶ���, ��Χ [-30000,30000]
  * @param[in]      back_R: (0x208) 6020������Ƶ���, ��Χ [-30000,30000]
  * @retval         none
  */
void CAN_cmd_rudder(int16_t forward_L, int16_t forward_R, int16_t back_L,int16_t back_R)
{
    uint32_t send_mail_box;
    rudder_tx_message.StdId = CAN_GIMBAL_ALL_ID;
    rudder_tx_message.IDE = CAN_ID_STD;
    rudder_tx_message.RTR = CAN_RTR_DATA;
    rudder_tx_message.DLC = 0x08;
    rudder_can_send_data[0] = (forward_L >> 8);
    rudder_can_send_data[1] = forward_L;
    rudder_can_send_data[2] = (forward_R >> 8);
    rudder_can_send_data[3] = forward_R;
    rudder_can_send_data[4] = (back_L >> 8);
    rudder_can_send_data[5] = back_L;
    rudder_can_send_data[6] = (back_R >> 8);
    rudder_can_send_data[7] = back_R;
    HAL_CAN_AddTxMessage(&RUDDER_CAN, &rudder_tx_message, rudder_can_send_data, &send_mail_box);
}


/**
  * @brief          �����ֵ�����Ƶ���(0x201,0x202,0x203,0x204)
  * @author         XQL
  * @param[in]      motor1: (0x201) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor2: (0x202) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor3: (0x203) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor4: (0x204) 3508������Ƶ���, ��Χ [-16384,16384]
  * @retval         none
  */
void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = CAN_CHASSIS_ALL_ID;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = motor1 >> 8;
    chassis_can_send_data[1] = motor1;
    chassis_can_send_data[2] = motor2 >> 8;
    chassis_can_send_data[3] = motor2;
    chassis_can_send_data[4] = motor3 >> 8;
    chassis_can_send_data[5] = motor3;
    chassis_can_send_data[6] = motor4 >> 8;
    chassis_can_send_data[7] = motor4;

    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}



/**
  * @brief          ���ͳ������ݳ�繦��
  * @author         XQL
  * @param[in]      temPower��0x210������繦�ʣ�45-100W��
  * @retval         none
  */
void CAN_CMD_cap(int16_t temPower)
{
		uint32_t send_mail_box;
    capid_tx_message.StdId = 0x210;
    capid_tx_message.IDE = CAN_ID_STD;
    capid_tx_message.RTR = CAN_RTR_DATA;
	  capid_tx_message.DLC = 0x08;
    capid_can_send_data[0] = temPower >> 8;
    capid_can_send_data[1] = temPower;      
   	HAL_CAN_AddTxMessage( &CAPID_CAN,  &capid_tx_message, capid_can_send_data, &send_mail_box);
}


/** 
  * @brief          ���ض��� 6020�������ָ��
  * @author         XQL
  * @param[in]      none
  * @retval         �������ָ��
  */
const motor_measure_t *get_Forward_L_motor_measure_point(void)
{
    return &Forward_L;
}
const motor_measure_t *get_Forward_R_motor_measure_point(void)
{
	return &Forward_R;
}
const motor_measure_t *get_Back_R_motor_measure_point(void)
{
	return &Back_R;
}
const motor_measure_t *get_Back_L_motor_measure_point(void)
{
	return &Back_L;
}


/** 
  * @brief          ���س�����������ָ��
  * @author         XQL
  * @param[in]      none
  * @retval         ������������ָ��
  */
 cap_measure_t *get_cap_measure_point(void)
{
    return &get_cap;
}


/**
  * @brief          �����ֵ�� 3508�������ָ��
  * @author         XQL
  * @param[in]      i: ������,��Χ[0,3]
  * @retval         �������ָ��
  */
const motor_measure_t *get_chassis_motor_measure_point(uint8_t i)
{
    return &motor_chassis[(i & 0x03)];
}
