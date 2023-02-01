#ifndef CHASSISTASK_H
#define CHASSISTASK_H

#include "Remote_Control.h"
#include "pid.h"
#include "CAN_Receive.h"
#include "user_lib.h"
#include "stm32.h"


//ǰ���ң����ͨ������
#define CHASSIS_X_CHANNEL 1
//���ҵ�ң����ͨ������
#define CHASSIS_Y_CHANNEL 2
//������ģʽ�£�����ͨ��ң����������ת
#define CHASSIS_WZ_CHANNEL 2

#define CHASSIS_RC_DEADLINE 10

#define TRIGGER_BULLET_PID_MAX_OUT 15000.0f
#define TRIGGER_BULLET_PID_MAX_IOUT 5000.0f

#define MOTOR_SPEED_TO_CHASSIS_SPEED_VX 0.2f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_VY 0.2f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_WZ 0.2f
#define MOTOR_DISTANCE_TO_CENTER   0.32 //0.32f

//����������Ƽ�� 2ms
#define CHASSIS_CONTROL_TIME_MS 1
//����������Ƽ�� 0.002s
#define CHASSIS_CONTROL_TIME 0.002

//���̵�ͨ�˲�ϵ��
#define CHASSIS_ACCEL_X_NUM 0.1666666667f
#define CHASSIS_ACCEL_Y_NUM 0.3333333333f

//X��Y�Ƕ���ң�����������
#define X_RC_SEN   0.0006f
#define Y_RC_SEN -0.0005f //0.005

//Y,Y�ǶȺ��������ı���
#define X_Mouse_Sen 0.0002f
#define Y_Mouse_Sen 0.00025f

//X,Y����ͨ��
#define X_Channel 2
#define Y_Channel 3

//�������ֵ����Լ���ֵ
#define Half_ecd_range 395  //395  7796
#define ecd_range 8191

//����-����
#ifndef Motor_Ecd_to_Rad
#define Motor_Ecd_to_Rad 0.00076708402f //      2*  PI  /8192
#endif

//���̵������ٶ�
#define MAX_WHEEL_SPEED 4.5f

//����3508���can���͵���ֵ
#define MAX_MOTOR_CAN_CURRENT 16000.0f
//���̵���ٶȻ�PID
#define CHASSIS_KP 6.f  //78.6432--62.91456
#define CHASSIS_KI 0.5f //118098f
#define CHASSIS_KD 30.f //
#define CHASSIS_MAX_OUT 10000.f
#define CHASSIS_MAX_IOUT 3000.f

//����PID
#define RUDDER_P_P 40.f 
#define RUDDER_P_I 0.f 
#define RUDDER_P_D 0.f
#define RUDDER_P_N 0.f
#define RUDDER_S_P 3.f
#define RUDDER_S_I 0.f
#define RUDDER_S_D 0.f
#define RUDDER_S_N 0.f

//������ת����PID
#define CHASSIS_FOLLOW_GIMBAL_PID_KP  15     //8.f
#define CHASSIS_FOLLOW_GIMBAL_PID_KI 0.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KD 1000.f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT  5000  //8.5
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT 0.5f
#define CHASSIS_FOLLOW_GIMBAL_PID_KF 0.f
#define CHASSIS_FOLLOW_GIMBAL_F_divider 0.0
#define CHASSIS_FOLLOW_GIMBAL_F_out_limit 00.f

//���̵�����ʻ�PID
#define M3505_MOTOR_POWER_PID_KP 2000.f
#define M3505_MOTOR_POWER_PID_KI 85.f
#define M3505_MOTOR_POWER_PID_KD 0.f
#define M3505_MOTOR_POWER_PID_MAX_OUT 60000.0f
#define M3505_MOTOR_POWER_PID_MAX_IOUT 900.0f

//m3508ת���ɵ����ٶ�(m/s)�ı������������� ����Ϊ���ܻ������Ҫ��������
#define M3508_MOTOR_RPM_TO_VECTOR 0.000415809748903494517209f


//m3508����ļ��ٱ�
#define M3508_MOTOR_REDUCATION 19.2032f

//m3508 rpm change to chassis speed
//m3508ת��ת��(rpm)ת���ɵ����ٶ�(m/s)�ı�����c=pi*r/(30*k)��kΪ������ٱ�
#define CHASSIS_MOTOR_RPM_TO_VECTOR_SEN M3508_MOTOR_RPM_TO_VECTOR


//m3508ת��ת��(rpm)ת��Ϊ�������ٶ�(rad/s)�ı���
#define CHASSIS_MOTOR_RPM_TO_OMG_SEN 0.0054533f


//m3508ת�ص���(-16384~16384)תΪ�ɵ�����ת��(N.m)�ı���
//c=20/16384*0.3��0.3Ϊת�س���(N.m/A)
#define CHASSIS_MOTOR_CURRENT_TO_TORQUE_SEN 0.000366211f


//�������̵���������
#define MAX_WHEEL_TORQUE 5.5f



//�����������Ƶ�ʣ���δʹ�������
#define CHASSIS_CONTROL_FREQUENCE 500.0f

//ң����ǰ��ҡ�ˣ�max 660��ת���ɳ���ǰ���ٶȣ�m/s���ı���
#define CHASSIS_VX_RC_SEN 0.006f

//ң��������ҡ�ˣ�max 660��ת���ɳ��������ٶȣ�m/s���ı���
#define CHASSIS_VY_RC_SEN 0.005f

//��������̨��ʱ�� ң������yawң�ˣ�max 660��ת���ɳ�����ת�ٶȵı���
#define CHASSIS_WZ_RC_SEN 0.01f

//����������ת�ٶȣ�����ǰ�������ֲ�ͬ�趨�ٶȵı�����Ȩ 0Ϊ�ڼ������ģ�����Ҫ����
#define CHASSIS_WZ_SET_SCALE 0.f

typedef enum
{
  CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW,   //���̸�����̨
  CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW,  //��������
	CHASSIS_VECTOR_SPIN,                //С����
  CHASSIS_VECTOR_NO_FOLLOW_YAW,       //���̲�����
  CHASSIS_VECTOR_RAW,									//����ԭʼ����

} chassis_mode_e;

typedef struct
{
  fp32 relative_angle;
} Gimbal_data;
typedef struct
{
    fp32 kp;
    fp32 ki;
    fp32 kd;
	
    fp32 set;
    fp32 get;
    fp32 err;

    fp32 max_out;
    fp32 max_iout;
	
    fp32 Pout;
    fp32 Iout;
    fp32 Dout;
	 
    fp32 out;
		
} chassis_PID_t;
typedef struct
{
    const motor_measure_t *gimbal_motor_measure;   //��̨����
	  Rudder_control rudder_control;
    uint16_t offset_ecd;
    fp32 max_relative_angle; //rad
    fp32 min_relative_angle; //rad
	  
	  fp32 control;
    fp32 relative_angle;     //rad
    fp32 relative_angle_set; //rad
    fp32 absolute_angle;     //rad
    fp32 absolute_angle_set; //rad
    fp32 motor_gyro;         //rad/s
    fp32 motor_gyro_set;    //���ٶ��趨
    fp32 motor_speed;     
    fp32 raw_cmd_current;
    fp32 current_set;
    int16_t given_current;
		fp32  ecd_add;       
		fp32 ecd_set;  
    fp32 ecd_set_final;	
		fp32 last_ecd_set;
		fp32 ecd_error;
		fp32 ecd_error_true;
		fp32 ecd_turn;
    fp32 ecd_change_MIN;
		fp32 wheel_speed;
		fp32 rudder_angle;
		fp32 last_rudder_angle;
		fp32 ecd_zero_set;
		fp32 Judge_Speed_Direction;
} Rudder_Motor_t;

typedef struct
{
  const motor_measure_t *chassis_motor_measure;
  fp32 accel;
  fp32 speed;
  fp32 speed_set;
  int16_t give_current;
	
	fp32 omg;     //�������ٶ�
	fp32 torque;  //���Ť��
	PidTypeDef chassis_pid;
} Chassis_Motor_t;

typedef struct
{        
	  const RC_ctrl_t *chassis_rc_ctrl;
	  Chassis_Motor_t motor_chassis[4]; 
	  chassis_mode_e chassis_motor_mode;
    chassis_mode_e last_chassis_motor_mode;
	 
	  PidTypeDef chassis_angle_pid;   //���̸���Ƕ�pid
  	const fp32 *gimbal_INT_angle_point;
    const fp32 *gimbal_INT_gyro_point;
	  
	  Gimbal_data gimbal_data;
    Rudder_Motor_t Forward_L;
    Rudder_Motor_t Forward_R;
	  Rudder_Motor_t Back_R;
	  Rudder_Motor_t Back_L; 
	  
	  fp32 vx;    //�����ٶ� ǰ������ ǰΪ������λ m/s
    fp32 vy;   //�����ٶ� ���ҷ��� ��Ϊ��  ��λ m/s
    fp32 wz;  //������ת���ٶȣ���ʱ��Ϊ�� ��λ rad/s
	
	  fp32 vx_set;      
    fp32 vy_set;                         
    fp32 wz_set;                         
		fp32 chassis_relative_angle_set; //���������̨���ƽǶ�
	
	  first_order_filter_type_t chassis_cmd_slow_set_vx;   // �˲�����
		first_order_filter_type_t chassis_cmd_slow_set_vy;
    
}chassis_move_t;


extern void chassis_task(void const *pvParameters);
extern void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector);

#endif
