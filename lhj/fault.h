#ifndef FAULT_H
#define FAULT_H

// 시퀀스 상태 정의
typedef enum {
    SEQ_INIT = 0,
    SEQ_WAIT = 1,
    SEQ_CHARGING = 2,
    SEQ_FAULT = 3,
    SEQ_RESET = 4,
    SEQ_SHUTDOWN = 5
} SeqState;

// 플러그 상태 정의
typedef enum {
    PLUG_UNPLUGGED = 0,
    PLUG_CONNECTED_NO_PAY = 1,
    PLUG_CONNECTED_PAID = 2
} PlugInfo;

// 고장 상태 정의
typedef enum {
    FAULT_NORMAL = 0,
    FAULT_DETECT = 1,
    FAULT_CONFIRM = 2
} FaultStatus;

// 고장 코드 ID 정의
typedef enum {
    FAULT_INPUT_OVERCURRENT = 0x01,
    FAULT_INPUT_UNDERCURRENT = 0x02,
    FAULT_PLUG = 0x03,
    FAULT_RELAY = 0x04,
    FAULT_BMS_STATE = 0x05,
    FAULT_OVER_TEMP = 0x06,
    FAULT_CAN = 0x07,
    FAULT_ISO = 0x08,
    FAULT_PAYMENT = 0x09,
    FAULT_WDT = 0x0A,
    FAULT_SEQ_TIMEOUT = 0x0B,
    FAULT_TEMP_SENSOR = 0x0C,
    FAULT_MAX
} FaultCode;

#include <stdint.h>
#include <math.h>
#include "input.h"

// 고장 코드 초기화
void Fault_Init(void);

// 종합 고장진단 함수(0x01 ~ 0x0C)
void Fault_Diagnose(const InputSnapshot* snapshot);

// 고장 진단 로직
void Diagnose_Fault_0x01(const InputSnapshot* in);
void Diagnose_Fault_0x02(const InputSnapshot* in);
void Diagnose_Fault_0x03(const InputSnapshot* in);
void Diagnose_Fault_0x04(const InputSnapshot* in);
void Diagnose_Fault_0x05(const InputSnapshot* in);
void Diagnose_Fault_0x06(const InputSnapshot* in);
void Diagnose_Fault_0x07(const InputSnapshot* in);
void Diagnose_Fault_0x08(const InputSnapshot* in);
void Diagnose_Fault_0x09(const InputSnapshot* in);
void Diagnose_Fault_0x0A(const InputSnapshot* in);
void Diagnose_Fault_0x0B(const InputSnapshot* in);
void Diagnose_Fault_0x0C(const InputSnapshot* in);

// 고장 진단 로직 단위 테스트
void Test_Fault_0x01(const char* csv_path);
void Test_Fault_0x02(const char* csv_path);
void Test_Fault_0x03(const char* csv_path);
void Test_Fault_0x04(const char* csv_path);
void Test_Fault_0x05(const char* csv_path);
void Test_Fault_0x06(const char* csv_path);
void Test_Fault_0x07(const char* csv_path);
void Test_Fault_0x08(const char* csv_path);
void Test_Fault_0x09(const char* csv_path);
void Test_Fault_0x0A(const char* csv_path);
void Test_Fault_0x0B(const char* csv_path);
void Test_Fault_0x0C(const char* csv_path);

/* 고장 상태 조회 */
FaultStatus Fault_GetStatus(FaultCode code);



#endif
