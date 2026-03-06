#ifndef INPUT_H
#define INPUT_H
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

/* ===== CSV 입력 1줄을 담는 구조체 ===== */
typedef struct
{
    int Cycle;
    int SeqState;
    int PlugInfo;
    int FLAG_Stop;
    int FLAG_Relay;

    float Ia;
    float Ib;
    float Ic;

    int FaultState;
    int Charg_Cnt;

    int Real_V;
    int Exp_V;
    int H;
    int CanMsg;
    int IsoR;
} InputSnapshot;

/* ===== CSV에서 한 줄 읽어서 구조체에 저장 ===== */
int Input_ReadLine(FILE* fp, InputSnapshot* out);

#endif /* INPUT_H */
