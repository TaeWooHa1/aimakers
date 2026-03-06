#include "fault.h"

/* 고장 상태 테이블 */
static FaultStatus faultState[FAULT_MAX];

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


// 고장 상태 초기화 함수
void Fault_Init(void)
{
    for (int i = 0; i < FAULT_MAX; i++) {
        faultState[i] = FAULT_NORMAL;
    }
}

// 종합 진단 코드(아직 통합 테스트는 미진행)
void Fault_Diagnose(const InputSnapshot* snapshot)
{
    Diagnose_Fault_0x01(snapshot);
    Diagnose_Fault_0x02(snapshot);
    Diagnose_Fault_0x03(snapshot);
    Diagnose_Fault_0x04(snapshot);
    Diagnose_Fault_0x05(snapshot);
    Diagnose_Fault_0x06(snapshot);
    Diagnose_Fault_0x07(snapshot);
    Diagnose_Fault_0x08(snapshot);
    Diagnose_Fault_0x09(snapshot);
    Diagnose_Fault_0x0A(snapshot);
    Diagnose_Fault_0x0B(snapshot);
    Diagnose_Fault_0x0C(snapshot);
}

FaultStatus Fault_GetStatus(FaultCode code)
{
    return faultState[code];
}

void Diagnose_Fault_0x01(const InputSnapshot* in)
{
    static uint8_t over_cnt = 0;
    static uint8_t rec_cnt = 0;

    /* ================= 고장 진단 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        in->Ia > 32 && in->Ib > 32 && in->Ic > 32)
    {
        if (over_cnt < 255)
            over_cnt++;

        rec_cnt = 0;

        /* DETECT 상태 */
        if (over_cnt < 10)
        {
            faultState[FAULT_INPUT_OVERCURRENT] = FAULT_DETECT;
        }
        else
        {
            /* CONFIRM 상태 */
            faultState[FAULT_INPUT_OVERCURRENT] = FAULT_CONFIRM;
        }
    }
    else
    {
        over_cnt = 0;

        /* ================= 고장 회복 ================= */
        if (faultState[FAULT_INPUT_OVERCURRENT] == FAULT_CONFIRM &&
            in->Ia < 24 && in->Ib < 24 && in->Ic < 24)
        {
            if (rec_cnt < 255)
                rec_cnt++;

            if (rec_cnt >= 10)
            {
                faultState[FAULT_INPUT_OVERCURRENT] = FAULT_NORMAL;
                rec_cnt = 0;
                over_cnt = 0;
            }
        }
        else
        {
            rec_cnt = 0;

            /* CONFIRM도 DETECT도 아니면 NORMAL 유지 */
            if (faultState[FAULT_INPUT_OVERCURRENT] != FAULT_CONFIRM)
            {
                faultState[FAULT_INPUT_OVERCURRENT] = FAULT_NORMAL;
            }
        }
    }
}

void Diagnose_Fault_0x02(const InputSnapshot* in)
{
    static uint8_t under_cnt = 0;
    static uint8_t rec_cnt = 0;

    /* ================= 진단 조건 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        in->Ia < 6 && in->Ib < 6 && in->Ic < 6)
    {
        if (under_cnt < 255)
            under_cnt++;

        rec_cnt = 0;

        if (under_cnt < 10)
        {
            faultState[FAULT_INPUT_UNDERCURRENT] = FAULT_DETECT;
        }
        else
        {
            faultState[FAULT_INPUT_UNDERCURRENT] = FAULT_CONFIRM;
        }
    }
    else
    {
        under_cnt = 0;

        /* ================= 회복 조건 ================= */
        if (faultState[FAULT_INPUT_UNDERCURRENT] == FAULT_CONFIRM &&
            in->Ia > 12 && in->Ib > 12 && in->Ic > 12)
        {
            if (rec_cnt < 255)
                rec_cnt++;

            if (rec_cnt >= 10)
            {
                faultState[FAULT_INPUT_UNDERCURRENT] = FAULT_NORMAL;
                under_cnt = 0;
                rec_cnt = 0;
            }
        }
        else
        {
            rec_cnt = 0;

            if (faultState[FAULT_INPUT_UNDERCURRENT] != FAULT_CONFIRM)
            {
                faultState[FAULT_INPUT_UNDERCURRENT] = FAULT_NORMAL;
            }
        }
    }
}

void Diagnose_Fault_0x03(const InputSnapshot* in)
{
    static uint8_t fault_cnt = 0;
    static uint8_t latched = 0;

    /* ================= 진단 조건 : 즉시 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        (
            in->PlugInfo == PLUG_UNPLUGGED ||
            (in->PlugInfo == PLUG_CONNECTED_PAID &&
                in->Ia < 0 && in->Ib < 0 && in->Ic < 0)
            ))
    {
        /* 처음 CONFIRM으로 들어갈 때만 카운트 */
        if (faultState[FAULT_PLUG] != FAULT_CONFIRM)
        {
            if (fault_cnt < 255)
                fault_cnt++;
        }

        faultState[FAULT_PLUG] = FAULT_CONFIRM;

        /* 3회 이상 → 재기동 금지 */
        if (fault_cnt >= 3)
        {
            latched = 1;
        }
    }
    else
    {
        /* ================= 회복 조건 ================= */
        if (!latched &&
            in->SeqState != SEQ_CHARGING &&
            in->PlugInfo == PLUG_CONNECTED_PAID)
        {
            faultState[FAULT_PLUG] = FAULT_NORMAL;
        }
        else
        {
            /* latched 상태이거나 회복 조건 미충족 → 유지 */
            if (faultState[FAULT_PLUG] != FAULT_CONFIRM)
            {
                faultState[FAULT_PLUG] = FAULT_NORMAL;
            }
        }
    }
}

void Diagnose_Fault_0x04(const InputSnapshot* in)
{
    /* ================= 진단 조건 : 즉시 ================= */
    if (
        (in->FLAG_Relay == 1 && in->FLAG_Stop == 1) ||
        (in->FLAG_Relay == 0 && in->FLAG_Stop == 0)
        )
    {
        faultState[FAULT_RELAY] = FAULT_CONFIRM;
    }
    else
    {
        /* ================= 회복 조건 : 즉시 ================= */
        if (
            (in->FLAG_Relay == 1 && in->FLAG_Stop == 0) ||
            (in->FLAG_Relay == 0 && in->FLAG_Stop == 1)
            )
        {
            faultState[FAULT_RELAY] = FAULT_NORMAL;
        }
    }
}

void Diagnose_Fault_0x05(const InputSnapshot* in)
{
    static uint8_t batt_cnt = 0;
    static uint8_t batt_rec_cnt = 0;

    int diff = abs(in->Real_V - in->Exp_V);

    /* ================= 진단 조건 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        in->Charg_Cnt > 10 &&
        diff > 10.0f)
    {
        if (batt_cnt < 255)
            batt_cnt++;

        batt_rec_cnt = 0;

        if (batt_cnt >= 10)
            faultState[FAULT_BMS_STATE] = FAULT_CONFIRM;
        else
            faultState[FAULT_BMS_STATE] = FAULT_DETECT;
    }
    else
    {
        batt_cnt = 0;

        /* ================= 회복 조건 ================= */
        if (faultState[FAULT_BMS_STATE] == FAULT_CONFIRM &&
            diff <= 5.0f)
        {
            if (batt_rec_cnt < 255)
                batt_rec_cnt++;

            if (batt_rec_cnt >= 10)
            {
                faultState[FAULT_BMS_STATE] = FAULT_NORMAL;
                batt_cnt = 0;
                batt_rec_cnt = 0;
            }
        }
        else
        {
            batt_rec_cnt = 0;

            if (faultState[FAULT_BMS_STATE] != FAULT_CONFIRM)
                faultState[FAULT_BMS_STATE] = FAULT_NORMAL;
        }
    }
}

void Diagnose_Fault_0x06(const InputSnapshot* in)
{
    static uint8_t heat_cnt = 0;
    static uint8_t heat_rec_cnt = 0;
    static uint8_t fault_cnt = 0;
    static uint8_t latched = 0;

    /* ================= 진단 조건 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        in->H > 60)
    {
        if (heat_cnt < 255)
            heat_cnt++;

        heat_rec_cnt = 0;

        if (heat_cnt >= 10)
        {
            /* CONFIRM 진입 시 1회만 카운트 */
            if (faultState[FAULT_OVER_TEMP] != FAULT_CONFIRM)
            {
                if (fault_cnt < 255)
                    fault_cnt++;
            }

            faultState[FAULT_OVER_TEMP] = FAULT_CONFIRM;

            /* 3회 이상 반복 → 재기동 금지 */
            if (fault_cnt >= 3)
            {
                latched = 1;
            }
        }
        else
        {
            faultState[FAULT_OVER_TEMP] = FAULT_DETECT;
        }
    }
    else
    {
        heat_cnt = 0;

        /* ================= 회복 조건 ================= */
        if (!latched &&
            in->SeqState == SEQ_INIT &&
            in->H < 20)
        {
            if (heat_rec_cnt < 255)
                heat_rec_cnt++;

            if (heat_rec_cnt >= 1)   /* 즉시 회복 허용 */
            {
                faultState[FAULT_OVER_TEMP] = FAULT_NORMAL;
                heat_rec_cnt = 0;
            }
        }
        else
        {
            heat_rec_cnt = 0;

            if (faultState[FAULT_OVER_TEMP] != FAULT_CONFIRM)
            {
                faultState[FAULT_OVER_TEMP] = FAULT_NORMAL;
            }
        }
    }
}

void Diagnose_Fault_0x07(const InputSnapshot* in)
{
    static uint8_t can_to_cnt = 0;
    static uint8_t can_rec_cnt = 0;

    /* ================= 진단 조건 ================= */
    if (in->CanMsg == 0)   /* CanMsg_Received == 0 */
    {
        if (can_to_cnt < 255)
            can_to_cnt++;

        can_rec_cnt = 0;

        if (can_to_cnt >= 5)
            faultState[FAULT_CAN] = FAULT_CONFIRM;
        else
            faultState[FAULT_CAN] = FAULT_DETECT;
    }
    else   /* CanMsg_Received == 1 */
    {
        can_to_cnt = 0;

        /* ================= 회복 조건 ================= */
        if (faultState[FAULT_CAN] == FAULT_CONFIRM)
        {
            if (can_rec_cnt < 255)
                can_rec_cnt++;

            if (can_rec_cnt >= 5)
            {
                faultState[FAULT_CAN] = FAULT_NORMAL;
                can_rec_cnt = 0;
            }
        }
        else
        {
            can_rec_cnt = 0;
            faultState[FAULT_CAN] = FAULT_NORMAL;
        }
    }
}

void Diagnose_Fault_0x08(const InputSnapshot* in)
{
    static uint8_t iso_cnt = 0;
    static uint8_t iso_rec_cnt = 0;

    /* ================= 진단 조건 ================= */
    if (in->SeqState == SEQ_CHARGING &&
        in->IsoR < 500)   /* IsoRmin */
    {
        if (iso_cnt < 255)
            iso_cnt++;

        iso_rec_cnt = 0;

        if (iso_cnt >= 10)
            faultState[FAULT_ISO] = FAULT_CONFIRM;
        else
            faultState[FAULT_ISO] = FAULT_DETECT;
    }
    else
    {
        iso_cnt = 0;

        /* ================= 회복 조건 ================= */
        if (faultState[FAULT_ISO] == FAULT_CONFIRM &&
            in->SeqState != SEQ_CHARGING &&
            in->IsoR > 600)   /* IsoRnormal */
        {
            if (iso_rec_cnt < 255)
                iso_rec_cnt++;

            if (iso_rec_cnt >= 10)
            {
                faultState[FAULT_ISO] = FAULT_NORMAL;
                iso_rec_cnt = 0;
            }
        }
        else
        {
            iso_rec_cnt = 0;

            if (faultState[FAULT_ISO] != FAULT_CONFIRM)
                faultState[FAULT_ISO] = FAULT_NORMAL;
        }
    }
}

void Diagnose_Fault_0x09(const InputSnapshot* in)
{
    static uint8_t pay_err_cnt = 0;

    /* ================= 회복 조건 (최우선) ================= */
    if ((in->PlugInfo == PLUG_CONNECTED_PAID &&
        in->SeqState == SEQ_CHARGING) ||
        (in->SeqState == SEQ_INIT))
    {
        faultState[FAULT_PAYMENT] = FAULT_NORMAL;
        pay_err_cnt = 0;
        return;
    }

    /* ================= 진단 조건 ================= */

    /* Case 2: 결제 미완료 상태 반복 */
    if (in->PlugInfo == PLUG_CONNECTED_NO_PAY)
    {
        if (pay_err_cnt < 255)
            pay_err_cnt++;

        if (pay_err_cnt >= 5)
            faultState[FAULT_PAYMENT] = FAULT_CONFIRM;
        else
            faultState[FAULT_PAYMENT] = FAULT_DETECT;
    }
    /* Case 1: 결제는 됐는데 충전이 시작되지 않음 */
    else if (in->PlugInfo == PLUG_CONNECTED_PAID &&
        in->SeqState != SEQ_CHARGING)
    {
        faultState[FAULT_PAYMENT] = FAULT_CONFIRM;
    }
}

void Diagnose_Fault_0x0A(const InputSnapshot* in)
{
    static int prev_cycle = -1;
    static int acc_delay = 0;   // 누적 지연 시간

    /* Latch fault */
    if (faultState[FAULT_WDT] == FAULT_CONFIRM)
        return;

    /* 첫 호출 */
    if (prev_cycle < 0)
    {
        prev_cycle = in->Cycle;
        return;
    }

    int diff = in->Cycle - prev_cycle;

    if (diff == 1)
    {
        /* 정상 heartbeat */
        acc_delay = 0;
    }
    else if (diff > 10)
    {
        /* 단발성 치명적 지연 */
        faultState[FAULT_WDT] = FAULT_CONFIRM;
        return;
    }
    else if (diff > 1)
    {
        /* 지연 누적 */
        acc_delay += diff;

        if (acc_delay >= 10)
        {
            faultState[FAULT_WDT] = FAULT_CONFIRM;
            return;
        }
    }
    else
    {
        /* diff <= 0 : 정지 / 역행 */
        acc_delay++;

        if (acc_delay >= 10)
        {
            faultState[FAULT_WDT] = FAULT_CONFIRM;
            return;
        }
    }

    prev_cycle = in->Cycle;
}

void Diagnose_Fault_0x0B(const InputSnapshot* in)
{
    static int prev_seq = -1;
    static int seq_timer = 0;              // 동일 시퀀스 체류 시간
    static uint8_t timeout_repeat_cnt = 0; // 고장 반복 횟수

    /* 재기동 중지 (Latch) */
    if (timeout_repeat_cnt >= 3)
    {
        faultState[FAULT_SEQ_TIMEOUT] = FAULT_CONFIRM;
        return;
    }

    /* 시퀀스 체류 시간 계산 */
    if (in->SeqState == prev_seq)
    {
        seq_timer++;
    }
    else
    {
        prev_seq = in->SeqState;
        seq_timer = 1;   // 진입 시 1초부터 카운트
    }

    int timeout = 0;

    /* 시퀀스별 타임아웃 조건 */
    switch (in->SeqState)
    {
    case SEQ_INIT:
    case SEQ_WAIT:
    case SEQ_FAULT:
    case SEQ_RESET:
        if (seq_timer >= 10)
            timeout = 1;
        break;

    case SEQ_CHARGING:
        if (seq_timer > 3600)   // Full_ChargeCnt
            timeout = 1;
        break;

    default:
        break;
    }

    /* 진단 즉시 */
    if (timeout)
    {
        faultState[FAULT_SEQ_TIMEOUT] = FAULT_CONFIRM;
        timeout_repeat_cnt++;
    }
    /* 회복 조건: fault 상태에서 INIT 진입 */
    else if (faultState[FAULT_SEQ_TIMEOUT] == FAULT_CONFIRM &&
        in->SeqState == SEQ_INIT)
    {
        faultState[FAULT_SEQ_TIMEOUT] = FAULT_NORMAL;
        seq_timer = 1;
    }
}

void Diagnose_Fault_0x0C(const InputSnapshot* in)
{
    static uint8_t temp_fault_cnt = 0;

    /* 진단 조건 */
    if ((in->SeqState == SEQ_CHARGING || in->SeqState == SEQ_INIT) && 
        (in->H < -20 || in->H > 120))
    {
        temp_fault_cnt++;

        if (temp_fault_cnt >= 3)
        {
            faultState[FAULT_TEMP_SENSOR] = FAULT_CONFIRM;
        }
        else
        {
            faultState[FAULT_TEMP_SENSOR] = FAULT_DETECT;
        }
    }
    /* 회복 조건 */
    else if (faultState[FAULT_TEMP_SENSOR] == FAULT_CONFIRM &&
        (in->SeqState == SEQ_INIT || in->SeqState == SEQ_CHARGING) &&
        (in->H >= -20 && in->H <= 120))
    {
        faultState[FAULT_TEMP_SENSOR] = FAULT_NORMAL;
        temp_fault_cnt = 0;
    }
    /* 그 외 */
    else
    {
        /* 유지 */
    }
}












