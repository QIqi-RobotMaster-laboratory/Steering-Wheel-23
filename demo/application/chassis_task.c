#include "chassis_task.h"   
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_laser.h"
#include "pid.h"
#include "Remote_Control.h"
#include "pid.h"
#include "arm_math.h"
#include "INS_Task.h"
#include "CAN_Receive.h"
#include "chassis_behaviour.h"
#include "stm32.h"
extern cap_measure_t get_cap;

#define pi 3.1415926
#define half_pi 1.5707963

/** 
  * @brief          ��������
	* @author         XQL
  * @param[in]      input��  ����ֵ
	* @param[in]      output�� ���ֵ
	* @param[in]      dealine������ֵ
  * @retval         none
  */
#define rc_deadline_limit(input, output, dealine)        \
    {                                                    \
        if ((input) > (dealine) || (input) < -(dealine)) \
        {                                                \
            (output) = (input);                          \
        }                                                \
        else                                             \
        {                                                \
            (output) = 0;                                \
        }                                                \
    }

			
/** 
  * @brief         ȡ����ֵ
	* @author        XQL
  * @param[in]     ����
  * @retval        none
  */		
#define abs(x) ((x) > 0 ? (x) : (-x))

		
/** 
  * @brief          �������ֵ���� 0��8191
	* @author         XQL
  * @param[in]      �������ֵ
  * @retval         none
  */		
#define ECD_Format(ecd)         \
    {                           \
        if ((ecd) > ecd_range)  \
            (ecd) -= ecd_range; \
        else if ((ecd) < 0)     \
            (ecd) += ecd_range; \
    }                           \
	
	
/**
  * @brief 	        ����ת����
  * @author         XQL
  * @param[in]      angle������ֵ
  * @retval         none
  */
#define rand(angle)  angle=angle/8191*3.1415926f*2 

	
/**
  * @brief 	        ȡ��Сֵ
	* @author         XQL
  * @param[in]      angle1������Ƕ�1
	* @param[in]      angle2������Ƕ�2
	* @param[in]      ture��  ����Ƕ�
  * @retval         none
  */
#define compare(angle1,angle2,ture) \
   {                                \
      if(angle1>angle2)             \
	     {                            \
         ture=angle2;               \
	     }                            \
      else                          \
      {                             \
	      ture=angle1;                \
	    }	                            \
   }	                            
 
	 
/**
  * @brief 	        �Ƕ�ת����
  * @author         XQL
  * @param[in]      angle������Ƕ�
  * @retval         none
  */                            
#define rad(angle) angle=angle/180*3.1415926f  

//���̿��������������
chassis_move_t chassis_move;	 

/** 
  * @brief          ���ض��� 6020�������ָ��
	* @author         XQL
  * @param[in]      none
  * @retval         �������ָ��
  */
const Rudder_Motor_t *get_Forward_L_motor_point(void)
{
    return &chassis_move.Forward_L;
}
const Rudder_Motor_t *get_Forward_R_motor_point(void)
{
	return &chassis_move.Forward_R;
}
const Rudder_Motor_t *get_Back_R_motor_point(void)
{
	return &chassis_move.Back_R;
}
const Rudder_Motor_t *get_Back_L_motor_point(void)
{
	return &chassis_move.Back_L;
}

//����PID��ʼ��
static void RUDDER_PID_INIT(chassis_move_t*rudder_init,const fp32 PID[8]);
//�ֵ���ٶ�����
static void chassis_speed_control_set(chassis_move_t*chassis_speed_set);
//���̳�ʼ��
static void chassis_init(chassis_move_t *chassis_move_init);
//�������ݸ���
static void chassis_feedback_update(chassis_move_t *chassis_move_update);
//����ģʽ����
static void chassis_set_mode(chassis_move_t *chassis_move_mode);
//�����л�ģʽ״̬����
static void chassis_mode_change_control_transit(chassis_move_t *chassis_move_transit);
//���̿���������
static void chassis_set_contorl(chassis_move_t *chassis_move_control);
//�֣���Ƕȣ��ٶȳ�������
static void chassic_rudder_preliminary_A_S_solution(chassis_move_t*chassic_rudder_preliminary_solution);
//���������
static void rudder_control_loop(chassis_move_t *rudder_move_control_loop);
//�ֵ���������
static void CHASSIC_MOTOR_PID_CONTROL(chassis_move_t *chassis_motor);
//��������Ƕȿ��������㼰�ٶȷ���ȷ��
static void rudder_MIN_angle_compute(Rudder_Motor_t *rudder_min_angle_comute);
//�����������
static void RUDDER_MOTOR_PID_CONTROL(Rudder_Motor_t *rudder_motor);
//���������
static void Rudder_motor_relative_angle_control(Rudder_Motor_t *chassis_motor);
//ң��������������������
void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector);
#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t chassis_high_water;
#endif


void chassis_task(void const *pvParameters)
{   
	
    
		chassis_init(&chassis_move);
		chassis_move.Forward_L.ecd_zero_set=3000;  
		chassis_move.Forward_R.ecd_zero_set=776;  
		chassis_move.Back_R.ecd_zero_set=4642;  
		chassis_move.Back_L.ecd_zero_set=4865;  
	
		while(1)
		{	 
		chassis_set_mode(&chassis_move); 
		chassis_mode_change_control_transit(&chassis_move);
		chassis_feedback_update(&chassis_move);	
		chassis_set_contorl(&chassis_move);
		rudder_control_loop(&chassis_move);
    CHASSIC_MOTOR_PID_CONTROL(&chassis_move);
			
    CAN_cmd_rudder(chassis_move.Forward_L.given_current,chassis_move.Forward_R.given_current,
			             chassis_move.Back_L.given_current,chassis_move.Back_R.given_current);	
	
//		CAN_cmd_chassis(chassis_move.motor_chassis[0].give_current,chassis_move.motor_chassis[1].give_current,
//										chassis_move.motor_chassis[2].give_current, chassis_move.motor_chassis[3].give_current);
		
		vTaskDelay(2);
#if INCLUDE_uxTaskGetStackHighWaterMark
        chassis_high_water = uxTaskGetStackHighWaterMark(NULL);
#endif			
		}
}

fp32 motor_speed_pid[3]={CHASSIS_KP, CHASSIS_KI, CHASSIS_KD};
const static fp32 chassis_yaw_pid[3] = {CHASSIS_FOLLOW_GIMBAL_PID_KP, CHASSIS_FOLLOW_GIMBAL_PID_KI, CHASSIS_FOLLOW_GIMBAL_PID_KD};		
fp32 rudder_pid[8]={RUDDER_P_P, RUDDER_P_I, RUDDER_P_D,RUDDER_P_N,RUDDER_S_P,RUDDER_S_I,RUDDER_S_D,RUDDER_S_N};

/** 
  * @brief          ��ʼ����������
  * @author         XQL
  * @param[in]      chassis_move_init����������ָ��
  * @retval         none
  */
static void chassis_init(chassis_move_t *chassis_move_init)
{
	

	  if (chassis_move_init == NULL)
    {
        return;
    }
		const static fp32 chassis_x_order_filter[1] = {CHASSIS_ACCEL_X_NUM};
    const static fp32 chassis_y_order_filter[1] = {CHASSIS_ACCEL_Y_NUM};
	  //��������ָ���ȡ
	  chassis_move_init->Forward_L.gimbal_motor_measure = get_Forward_L_motor_measure_point(); 
	  chassis_move_init->Forward_R.gimbal_motor_measure = get_Forward_R_motor_measure_point(); 
	  chassis_move_init->Back_R.gimbal_motor_measure = get_Back_R_motor_measure_point(); 
	  chassis_move_init->Back_L.gimbal_motor_measure = get_Back_L_motor_measure_point(); 
  
		chassis_move_init->chassis_motor_mode = CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW;
		
		//�ֵ������ָ���ȡ��PID��ʼ��
    int i;		
	  for(i=0;i<4;i++)
     {	
	   chassis_move_init->motor_chassis[i].chassis_motor_measure = get_chassis_motor_measure_point(i);
     PID_Init(&chassis_move_init->motor_chassis[i].chassis_pid, PID_POSITION, motor_speed_pid, CHASSIS_MAX_OUT, CHASSIS_MAX_IOUT);
     }

		 //����PID��ʼ��
		 RUDDER_PID_INIT(chassis_move_init,rudder_pid);
		 //������̨PID
		 PID_Init(&chassis_move_init->chassis_angle_pid, PID_POSITION, chassis_yaw_pid, CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT, CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT);
     //ң��������ָ���ȡ
     chassis_move_init->chassis_rc_ctrl = get_remote_control_point();
		 
		 first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vx, CHASSIS_CONTROL_TIME, chassis_x_order_filter);
     first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vy, CHASSIS_CONTROL_TIME, chassis_y_order_filter);
		 
		 chassis_move_init->Forward_L.Judge_Speed_Direction=chassis_move_init->Forward_R.Judge_Speed_Direction=
		 chassis_move_init->Back_L.Judge_Speed_Direction=chassis_move_init->Back_R.Judge_Speed_Direction=1.0f;
		 
     chassis_feedback_update(chassis_move_init);
}


/** 
  * @brief          ���µ�������
  * @author         XQL
  * @param[in]      chassis_move_update����������ָ��
  * @retval         none
  */
static void chassis_feedback_update(chassis_move_t *chassis_move_update)
{
    if (chassis_move_update == NULL)
    {
        return;
    }	
		
    uint8_t i = 0;
    for (i = 0; i < 4; i++)
    {   
    chassis_move_update->motor_chassis[i].speed = CHASSIS_MOTOR_RPM_TO_VECTOR_SEN * chassis_move_update->motor_chassis[i].chassis_motor_measure->speed_rpm;
		
			//update motor angular velocity
			//���µ���������ٶ�
		chassis_move_update->motor_chassis[i].omg = CHASSIS_MOTOR_RPM_TO_OMG_SEN * chassis_move_update->motor_chassis[i].chassis_motor_measure->speed_rpm;
			
			//update motor torque
			//���µ��ת��
		chassis_move_update->motor_chassis[i].torque = CHASSIS_MOTOR_CURRENT_TO_TORQUE_SEN * chassis_move_update->motor_chassis[i].chassis_motor_measure->given_current;
    }	
}


/** 
  * @brief          ���̿���ģʽ����
  * @author         XQL
  * @param[in]      chassis_move_mode����������ָ��
  * @retval         none
  */
static void chassis_set_mode(chassis_move_t *chassis_move_mode)
{
    if (chassis_move_mode == NULL)
    {
        return;
    }
	  chassis_behaviour_mode_set(chassis_move_mode);
}


/** 
  * @brief          �����л�ģʽ���ݻ���
  * @author         XQL
  * @param[in]      chassis_move_transit����������ָ��
  * @retval         none
  */
static void chassis_mode_change_control_transit(chassis_move_t *chassis_move_transit)
{
    if (chassis_move_transit == NULL||chassis_move_transit->last_chassis_motor_mode == chassis_move_transit->chassis_motor_mode)
    {
        return;
    }

    if ((chassis_move_transit->last_chassis_motor_mode != CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW) && 
			   chassis_move_transit->chassis_motor_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
    {
         chassis_move_transit->chassis_relative_angle_set = 0.0f;
    }
    else if ((chassis_move_transit->last_chassis_motor_mode != CHASSIS_VECTOR_SPIN) && 
			        chassis_move_transit->chassis_motor_mode == CHASSIS_VECTOR_SPIN)
    {
         chassis_move_transit->chassis_relative_angle_set = 0.0f;
    } 
	  else if ((chassis_move_transit->last_chassis_motor_mode == CHASSIS_VECTOR_SPIN) && 
			        chassis_move_transit->chassis_motor_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
    {
         chassis_move_transit->chassis_relative_angle_set = 0.0f;
    }
    chassis_move_transit->last_chassis_motor_mode = chassis_move_transit->chassis_motor_mode;
}
 

/** 
  * @brief          ����������������ü�ת��
  * @author         XQL
  * @param[in]      chassis_move_control����������ָ��
  * @retval         none
  */
static void chassis_set_contorl(chassis_move_t *chassis_move_control)
{
    if (chassis_move_control == NULL)
    {
        return;
    }
	
    fp32 vx_set = 0.0f, vy_set = 0.0f, angle_set = 0.0f;
    fp32 sin_yaw,cos_yaw=0.0f;	   fp32 relative_angle=0.0f;	
    chassis_behaviour_control_set(&vx_set, &vy_set, &angle_set, chassis_move_control);
		
		if(chassis_move_control->chassis_motor_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
		{   
			  chassis_move_control->chassis_relative_angle_set = rad_format(0.0f);
			  relative_angle=chassis_move_control->gimbal_data.relative_angle=0.0f;
			
			  if(relative_angle>PI) relative_angle=-2*PI+relative_angle;
			  
			  sin_yaw = arm_sin_f32((relative_angle));
        cos_yaw = arm_cos_f32((relative_angle));
			
        chassis_move_control->vx_set= cos_yaw * vx_set - sin_yaw * vy_set;
        chassis_move_control->vy_set = sin_yaw * vx_set + cos_yaw * vy_set;
        chassis_move_control->wz_set = -PID_Calc(&chassis_move_control->chassis_angle_pid, relative_angle, chassis_move_control->chassis_relative_angle_set); 
		}
		else if(chassis_move_control->chassis_motor_mode==CHASSIS_VECTOR_SPIN)
		{
			  relative_angle=chassis_move_control->gimbal_data.relative_angle;
			
			  if(relative_angle>PI) relative_angle=-2*PI+relative_angle;
			
			  sin_yaw = arm_sin_f32((relative_angle));
        cos_yaw = arm_cos_f32((relative_angle));
			
        chassis_move_control->vx_set= cos_yaw * vx_set - sin_yaw * vy_set;
        chassis_move_control->vy_set = sin_yaw * vx_set + cos_yaw * vy_set;
			  chassis_move_control->wz_set=0.0f;
		}
		else if (chassis_move_control->chassis_motor_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW)
    {
        chassis_move_control->vx_set = vx_set;
        chassis_move_control->vy_set =vy_set;
			  chassis_move_control->wz_set = angle_set;
    }
    else if (chassis_move_control->chassis_motor_mode == CHASSIS_VECTOR_RAW)
    {
        chassis_move_control->vx_set = vx_set;
        chassis_move_control->vy_set = vy_set;
        chassis_move_control->wz_set = angle_set;
    }
		
		chassic_rudder_preliminary_A_S_solution(chassis_move_control);
}


/** 
  * @brief          �֣�������Ƕ��ٶȿ���������
  * @author         XQL
  * @param[in]      chassic_rudder_preliminary_solution����������ָ��
  * @retval         none
  */
static void chassic_rudder_preliminary_A_S_solution(chassis_move_t*chassic_rudder_preliminary_solution)
{
	  fp32 vx_set,vy_set,vw_set=0.0f;
	  vx_set=chassic_rudder_preliminary_solution->vx_set;
	  vy_set=chassic_rudder_preliminary_solution->vy_set;
	  vw_set=chassic_rudder_preliminary_solution->wz_set;
	
	  chassic_rudder_preliminary_solution->Forward_L.last_rudder_angle=chassic_rudder_preliminary_solution->Forward_L.rudder_angle;
	  chassic_rudder_preliminary_solution->Back_L.last_rudder_angle=chassic_rudder_preliminary_solution->Back_L.rudder_angle;
	  chassic_rudder_preliminary_solution->Back_R.last_rudder_angle=chassic_rudder_preliminary_solution->Back_R.rudder_angle;
	  chassic_rudder_preliminary_solution->Forward_R.last_rudder_angle=chassic_rudder_preliminary_solution->Forward_R.rudder_angle; 
	
    chassic_rudder_preliminary_solution->Forward_L.wheel_speed=sqrt((pow((vy_set-vw_set*arm_cos_f32(45)),2)+pow((vx_set-vw_set*arm_sin_f32(45)),2)));
		chassic_rudder_preliminary_solution->Back_L.wheel_speed=sqrt((pow((vy_set-vw_set*arm_cos_f32(45)),2)+pow((vx_set+vw_set*arm_sin_f32(45)),2)));
		chassic_rudder_preliminary_solution->Back_R.wheel_speed=sqrt((pow((vy_set+vw_set*arm_cos_f32(45)),2)+pow((vx_set+vw_set*arm_sin_f32(45)),2)));
		chassic_rudder_preliminary_solution->Forward_R.wheel_speed=sqrt((pow((vy_set+vw_set*arm_cos_f32(45)),2)+pow((vx_set-vw_set*arm_sin_f32(45)),2)));
		                                                                                                                              //��ʱ����ת
		chassic_rudder_preliminary_solution->Forward_L.rudder_angle=atan2((vy_set-vw_set*arm_cos_f32(45)),(vx_set-vw_set*arm_sin_f32(45)));                   // 0  3 
		chassic_rudder_preliminary_solution->Back_L.rudder_angle=atan2((vy_set-vw_set*arm_cos_f32(45)),(vx_set+vw_set*arm_sin_f32(45)));                   // 1  2
		chassic_rudder_preliminary_solution->Back_R.rudder_angle=atan2((vy_set+vw_set*arm_cos_f32(45)),(vx_set+vw_set*arm_sin_f32(45)));
	  chassic_rudder_preliminary_solution->Forward_R.rudder_angle=atan2((vy_set+vw_set*arm_cos_f32(45)),(vx_set-vw_set*arm_sin_f32(45)));	
    
		chassic_rudder_preliminary_solution->Forward_L.ecd_add = chassic_rudder_preliminary_solution->Forward_L.rudder_angle/Motor_Ecd_to_Rad;
		chassic_rudder_preliminary_solution->Back_L.ecd_add = chassic_rudder_preliminary_solution->Back_L.rudder_angle/Motor_Ecd_to_Rad;
		chassic_rudder_preliminary_solution->Back_R.ecd_add = chassic_rudder_preliminary_solution->Back_R.rudder_angle/Motor_Ecd_to_Rad;
		chassic_rudder_preliminary_solution->Forward_R.ecd_add = chassic_rudder_preliminary_solution->Forward_R.rudder_angle/Motor_Ecd_to_Rad;
}


/** 
  * @brief          �ֵ���ٶ�����
  * @author         XQL
  * @param[in]      chassis_speed_set����������ָ��
  * @retval         none
  */
static void chassis_speed_control_set(chassis_move_t*chassis_speed_set)
{ 
	
	chassis_speed_set->motor_chassis[0].speed_set=chassis_speed_set->Forward_L.wheel_speed*chassis_speed_set->Forward_L.Judge_Speed_Direction;
	chassis_speed_set->motor_chassis[1].speed_set=chassis_speed_set->Forward_R.wheel_speed*chassis_speed_set->Forward_R.Judge_Speed_Direction;
	chassis_speed_set->motor_chassis[2].speed_set=chassis_speed_set->Back_L.wheel_speed*chassis_speed_set->Back_L.Judge_Speed_Direction;
	chassis_speed_set->motor_chassis[3].speed_set=chassis_speed_set->Back_R.wheel_speed*chassis_speed_set->Back_R.Judge_Speed_Direction;	
}


/** 
  * @brief          �����������������
  * @author         XQL
  * @param[in]      chassis_move_control_loop����������ָ��
  * @retval         none
  */
static void rudder_control_loop(chassis_move_t *rudder_move_control_loop)
{
    Rudder_motor_relative_angle_control(&rudder_move_control_loop->Forward_L);
	  Rudder_motor_relative_angle_control(&rudder_move_control_loop->Back_L);
	  Rudder_motor_relative_angle_control(&rudder_move_control_loop->Back_R);
	  Rudder_motor_relative_angle_control(&rudder_move_control_loop->Forward_R);
}


/** 
  * @brief          ��������������
  * @author         XQL
  * @param[in]      chassis_motor����������ָ��
  * @retval         none
  */
static void Rudder_motor_relative_angle_control(Rudder_Motor_t *chassis_motor)
{   
	//����Ŀ��λ��
	 if(chassis_motor->ecd_add>0)
	 {
			if(chassis_motor->ecd_zero_set+chassis_motor->ecd_add>8191)
			 {
					chassis_motor->ecd_set=chassis_motor->ecd_zero_set+chassis_motor->ecd_add-8191;
			 }
			else if(chassis_motor->ecd_zero_set+chassis_motor->ecd_add<8191)
			 {
					chassis_motor->ecd_set=chassis_motor->ecd_zero_set+chassis_motor->ecd_add;
			 }
   }
	else if(chassis_motor->ecd_add<0)
	{
			if(chassis_motor->ecd_zero_set+chassis_motor->ecd_add<0)
			 {
					chassis_motor->ecd_set=chassis_motor->ecd_zero_set+chassis_motor->ecd_add+8191;
			 }
			else if(chassis_motor->ecd_zero_set+chassis_motor->ecd_add>0)
			 {
					chassis_motor->ecd_set=chassis_motor->ecd_zero_set+chassis_motor->ecd_add;
			 }
	}
  else if(chassis_motor->ecd_add==0.0f)
  {
			chassis_motor->ecd_set= chassis_motor->ecd_zero_set;
  }
  
	chassis_motor->ecd_error=chassis_motor->ecd_set-chassis_motor->gimbal_motor_measure->ecd;
	
  //�ͽ�ԭ��
	if(chassis_motor->ecd_error>0)
	{
	  if(chassis_motor->ecd_error>4096)
		{
			chassis_motor->ecd_error=chassis_motor->ecd_error-8191;
		}
		else
		{
			chassis_motor->ecd_error_true=chassis_motor->ecd_error;
		}
	}
	else if(chassis_motor->ecd_error<0)
	{
		if(chassis_motor->ecd_error<-4096)
		{
			chassis_motor->ecd_error=8191+chassis_motor->ecd_error;
		}
	}
	else
	{
		chassis_motor->ecd_error=0;
	}
 // rudder_MIN_angle_compute(chassis_motor);
	RUDDER_MOTOR_PID_CONTROL(chassis_motor);
}


/** 
  * @brief          ��������Ƕȿ��������㼰�ٶȷ���ȷ��
	* @author         XQL
  * @param[in]      rudder_motor����������ָ��
  * @retval         none
  */
static void rudder_MIN_angle_compute(Rudder_Motor_t *rudder_min_angle_comute)
{
	//�ö�Ǳ���仯��0-2048�ڣ����Է����ٶȼ�С��ǣ�
	
		rudder_min_angle_comute->ecd_set_final=rudder_min_angle_comute->ecd_set+4096;
		
		if(rudder_min_angle_comute->ecd_set_final>=8191) 
		{
			rudder_min_angle_comute->ecd_set_final-=8191;
		}
  
	rudder_min_angle_comute->ecd_change_MIN=abs((rudder_min_angle_comute->ecd_set_final-rudder_min_angle_comute->gimbal_motor_measure->ecd));
	
	if(abs(rudder_min_angle_comute->ecd_error)>rudder_min_angle_comute->ecd_change_MIN)
	{ 
		rudder_min_angle_comute->ecd_change_MIN=rudder_min_angle_comute->ecd_set_final-rudder_min_angle_comute->gimbal_motor_measure->ecd;
		
		rudder_min_angle_comute->Judge_Speed_Direction=-1.0f;
	}
	else
	{
		rudder_min_angle_comute->ecd_change_MIN=rudder_min_angle_comute->ecd_error;
		
		rudder_min_angle_comute->Judge_Speed_Direction=1.0f;
	}	
}

/** 
  * @brief          ������������������
	* @author         XQL
  * @param[in]      rudder_motor����������ָ��
  * @retval         none
  */	
static void RUDDER_MOTOR_PID_CONTROL(Rudder_Motor_t *rudder_motor)   
{
	//Matlab_PID_Calc(rudder_motor->ecd_change_MIN,0,0,&rudder_motor->rudder_control);
	Matlab_PID_Calc(rudder_motor->ecd_error,0,0,&rudder_motor->rudder_control);
	
	rudder_motor->given_current=rudder_motor->rudder_control.rudder_out.Out1;
}


/** 
  * @brief          �ֵ����������������
	* @author         XQL
  * @param[in]      chassis_motor���ֵ������ָ��
  * @retval         none
  */	
static void CHASSIC_MOTOR_PID_CONTROL(chassis_move_t *chassis_motor)
{
	chassis_speed_control_set(chassis_motor);
	
	int i;
	
	for(i=0;i<4;i++)
	{   
	   chassis_motor->motor_chassis[i].give_current=PID_Calc(&chassis_motor->motor_chassis[i].chassis_pid,chassis_motor->motor_chassis[i].speed,chassis_motor->motor_chassis[i].speed_set);
	}
	
}


/** 
  * @brief          ����PID�γ�ʼ��
  * @author         XQL
  * @param[in]      rudder_init����������ָ��
  * @param[in]      PID[8]��PID����
  * @retval         none
  */	
static void RUDDER_PID_INIT(chassis_move_t*rudder_init,const fp32 PID[8])
{
	rudder_init->Forward_L.rudder_control.rudder_in.P_P=rudder_init->Forward_R.rudder_control.rudder_in.P_P=
	rudder_init->Back_L.rudder_control.rudder_in.P_P=rudder_init->Back_R.rudder_control.rudder_in.P_P=PID[0];
	
	rudder_init->Forward_L.rudder_control.rudder_in.P_I=rudder_init->Forward_R.rudder_control.rudder_in.P_I=
	rudder_init->Back_L.rudder_control.rudder_in.P_I=rudder_init->Back_R.rudder_control.rudder_in.P_I=PID[1];
	
	rudder_init->Forward_L.rudder_control.rudder_in.P_D=rudder_init->Forward_R.rudder_control.rudder_in.P_D=
	rudder_init->Back_L.rudder_control.rudder_in.P_D=rudder_init->Back_R.rudder_control.rudder_in.P_D=PID[2];
	
	rudder_init->Forward_L.rudder_control.rudder_in.P_N=rudder_init->Forward_R.rudder_control.rudder_in.P_N=
	rudder_init->Back_L.rudder_control.rudder_in.P_N=rudder_init->Back_R.rudder_control.rudder_in.P_N=PID[3];
	
	rudder_init->Forward_L.rudder_control.rudder_in.S_P=rudder_init->Forward_R.rudder_control.rudder_in.S_P=
	rudder_init->Back_L.rudder_control.rudder_in.S_P=rudder_init->Back_R.rudder_control.rudder_in.S_P=PID[4];
	
	rudder_init->Forward_L.rudder_control.rudder_in.S_I=rudder_init->Forward_R.rudder_control.rudder_in.S_I=
	rudder_init->Back_L.rudder_control.rudder_in.S_I=rudder_init->Back_R.rudder_control.rudder_in.S_I=PID[5];
	
	rudder_init->Forward_L.rudder_control.rudder_in.S_D=rudder_init->Forward_R.rudder_control.rudder_in.S_D=
	rudder_init->Back_L.rudder_control.rudder_in.S_D=rudder_init->Back_R.rudder_control.rudder_in.S_D=PID[6];
	
	rudder_init->Forward_L.rudder_control.rudder_in.S_N=rudder_init->Forward_R.rudder_control.rudder_in.S_N=
	rudder_init->Back_L.rudder_control.rudder_in.S_N=rudder_init->Back_R.rudder_control.rudder_in.S_N=PID[7];
}


/** 
  * @brief          ң����,��������������
  * @author         XQL
  * @param[in]      *vx_set��x�����ٶ�������
  * @param[in]      *vy_set��y�����ٶ�������
  * @param[in]      chassis_move_rc_to_vector����������ָ��
  * @retval         none
  */	
void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (chassis_move_rc_to_vector == NULL || vx_set == NULL || vy_set == NULL)
    {
        return;
    }
	
		//ң����
   	int16_t vx_channel_RC, vy_channel_RC;
	  fp32 vx_set_channel_RC, vy_set_channel_RC;
		/*//����
		int16_t vx_channel, vy_channel; 
	  fp32 vx_set_channel, vy_set_channel; */
		
    rc_deadline_limit(chassis_move_rc_to_vector->chassis_rc_ctrl->rc.ch[3], vx_channel_RC, CHASSIS_RC_DEADLINE);
    rc_deadline_limit(chassis_move_rc_to_vector->chassis_rc_ctrl->rc.ch[0], vy_channel_RC, CHASSIS_RC_DEADLINE);
	
	  vx_set_channel_RC = -vx_channel_RC * CHASSIS_VX_RC_SEN;
    vy_set_channel_RC = vy_channel_RC * CHASSIS_VY_RC_SEN;	
 
    // First order low pass filter
    first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vx, vx_set_channel_RC);
    first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vy, vy_set_channel_RC);

    if(vy_channel_RC>50)
		{
			*vy_set=1.0f;		
		}
	  else if(vy_channel_RC<-50)
		{
			*vy_set=-1.0f;	
		}
		else	
		{
			*vx_set=0.0f;
		}
	
		if(vx_channel_RC>50)
		{
			*vx_set =1.0f;		
		}
		else if(vx_channel_RC<-50)
		{
			 *vx_set=-1.0f;	
		}
	  else
		{
			 *vx_set=0.0f;	
		}
		
}
