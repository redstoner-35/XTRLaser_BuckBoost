#ifndef _LVProt_
#define _LvProt_

//内部包含
#include "stdbool.h"
#include "ModeControl.h"

//函数
void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump); //普通挡位的警报函数
void RampLowVoltHandler(void); //无极调光的专属处理
void BattAlertTIMHandler(void); //电池低电量报警处理函数
void RampRestoreLVProtToMax(void); //每次开机进入无级模式时尝试恢复限流

#endif
