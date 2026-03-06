#include "input.h"
#include <string.h>

int Input_ReadLine(FILE* fp, InputSnapshot* out)
{
    char line[256];
    static int first_call = 1;   // 처음 호출 여부

    /* 첫 호출이면 헤더 1줄 스킵 */
    if (first_call)
    {
        if (fgets(line, sizeof(line), fp) == NULL)
            return 0;   // 파일 비어있음

        first_call = 0;
    }

    /* 실제 데이터 한 줄 읽기 */
    if (fgets(line, sizeof(line), fp) == NULL)
        return 0;   // EOF

    /* CSV 파싱 */
    if (sscanf(line,
        "%d,%d,%d,%d,%d,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d",
        &out->Cycle,
        &out->SeqState,
        &out->PlugInfo,
        &out->FLAG_Stop,
        &out->FLAG_Relay,
        &out->Ia,
        &out->Ib,
        &out->Ic,
        &out->FaultState,
        &out->Charg_Cnt,
        &out->Real_V,
        &out->Exp_V,
        &out->H,
        &out->CanMsg,
        &out->IsoR
    ) != 15)
    {
        return 0;   // 파싱 실패 → 테스트 종료
    }

    return 1;       // 정상 데이터 1줄
}

