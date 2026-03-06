#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "fault.h"
#include "input.h"

static const char* FaultStateToStr(int state)
{
    switch (state)
    {
    case FAULT_NORMAL:  return "NORMAL ";
    case FAULT_DETECT:  return "DETECT ";
    case FAULT_CONFIRM: return "CONFIRM";
    default:            return "UNKNOWN";
    }
}

void Test_Fault_0x01(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | Ia Ib Ic | FaultState\n");
    printf("------------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x01(&in);

        int state = Fault_GetStatus(FAULT_INPUT_OVERCURRENT);

        printf("%5d | %3d | %2.0f %2.0f %2.0f | %s\n",
            in.Cycle,
            in.SeqState,
            in.Ia,
            in.Ib,
            in.Ic,
            FaultStateToStr(state));
    }

    fclose(fp);
}

void Test_Fault_0x02(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();   // 모든 fault 상태 초기화

    printf("Cycle | Seq | Ia Ib Ic | Fault_0x02\n");
    printf("------------------------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        /* ✅ 오직 이 함수만으로 판단 */
        Diagnose_Fault_0x02(&in);

        int state = Fault_GetStatus(FAULT_INPUT_UNDERCURRENT);

        printf("%5d | %8d | %2.0f %2.0f %2.0f | %s\n",
            in.Cycle,
            in.SeqState,
            in.Ia,
            in.Ib,
            in.Ic,
            FaultStateToStr(state));
    }

    fclose(fp);
}

/* ================= Fault 0x03 Test ================= */
void Test_Fault_0x03(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | Plug | Ia Ib Ic | Fault_0x03\n");
    printf("------------------------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x03(&in);

        FaultStatus st = Fault_GetStatus(FAULT_PLUG);

        printf("%5d | %3d | %4d | %2.0f %2.0f %2.0f | %s\n",
            in.Cycle,
            in.SeqState,
            in.PlugInfo,
            in.Ia,
            in.Ib,
            in.Ic,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x04(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Relay Stop | Fault_0x04\n");
    printf("---------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x04(&in);

        FaultStatus st = Fault_GetStatus(FAULT_RELAY);

        printf("%5d |   %d     %d  | %s\n",
            in.Cycle,
            in.FLAG_Relay,
            in.FLAG_Stop,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x05(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | ChgCnt | RealV ExpV | Fault_0x05\n");
    printf("------------------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x05(&in);

        FaultStatus st = Fault_GetStatus(FAULT_BMS_STATE);

        printf("%5d | %6d | %5.0d %5.0d | %s\n",
            in.Cycle,
            in.Charg_Cnt,
            in.Real_V,
            in.Exp_V,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x06(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | Temp(H) | Fault_0x06\n");
    printf("-----------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x06(&in);

        FaultStatus st = Fault_GetStatus(FAULT_OVER_TEMP);

        printf("%5d | %3d | %7d | %s\n",
            in.Cycle,
            in.SeqState,
            in.H,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x07(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | CanMsg | Fault_0x07\n");
    printf("-----------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x07(&in);

        FaultStatus st = Fault_GetStatus(FAULT_CAN);

        printf("%5d |   %d    | %s\n",
            in.Cycle,
            in.CanMsg,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x08(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | IsoR | Fault_0x08\n");
    printf("--------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x08(&in);

        FaultStatus st = Fault_GetStatus(FAULT_ISO);

        printf("%5d | %3d | %4d | %s\n",
            in.Cycle,
            in.SeqState,
            in.IsoR,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x09(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | Plug | Fault_0x09\n");
    printf("--------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x09(&in);

        FaultStatus st = Fault_GetStatus(FAULT_PAYMENT);

        printf("%5d | %3d | %4d | %s\n",
            in.Cycle,
            in.SeqState,
            in.PlugInfo,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x0A(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Fault_0x0A (WDT)\n");
    printf("------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x0A(&in);

        FaultStatus st = Fault_GetStatus(FAULT_WDT);

        printf("%5d | %s\n",
            in.Cycle,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x0B(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | Fault_0x0B\n");
    printf("------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x0B(&in);

        FaultStatus st = Fault_GetStatus(FAULT_SEQ_TIMEOUT);

        printf("%5d | %3d | %s\n",
            in.Cycle,
            in.SeqState,
            FaultStateToStr(st));
    }

    fclose(fp);
}

void Test_Fault_0x0C(const char* csv_path)
{
    FILE* fp;
    InputSnapshot in = { 0 };

    fp = fopen(csv_path, "r");
    if (!fp)
    {
        printf("CSV open failed\n");
        return;
    }

    Fault_Init();

    printf("Cycle | Seq | H | Fault_0x0C\n");
    printf("--------------------------------\n");

    while (Input_ReadLine(fp, &in))
    {
        Diagnose_Fault_0x0C(&in);

        FaultStatus st = Fault_GetStatus(FAULT_TEMP_SENSOR);

        printf("%5d | %3d | %4d | %s\n",
            in.Cycle,
            in.SeqState,
            in.H,
            FaultStateToStr(st));
    }

    fclose(fp);
}




