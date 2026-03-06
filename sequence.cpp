/*============================================================================
 * OBC (On-Board Charger) Control Sequence Example
 * 전기차 충전기 제어 시퀀스 예시
 * 
 * 정상 충전 플로우:
 * INIT(모든 변수 초기화) -> WAIT(플러그/결제 대기) -> CHARGING(충전 중)
 * -> 완료/고장 감지 시 FAULT -> RESET -> SHUTDOWN
 *
 *===========================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* ============ CAN Signal Definitions (CAN 신호 정의) ============ */

// 시퀀스 상태 정의
#define SEQ_INIT      0     // 초기화 상태 (릴레이 OFF, 충전 STOP)
#define SEQ_WAIT      1     // 대기 상태 (플러그 연결/결제 대기)
#define SEQ_CHARGING  2     // 정상 충전 상태 (전류 ramp-up, 정상 충전 중)
#define SEQ_FAULT     3     // 고장 감지 상태 (보호 로직 작동, 강제 STOP)
#define SEQ_RESET     4     // 복구 시도 상태
#define SEQ_SHUTDOWN  5     // 종료 상태

// 플러그/결제 상태
#define PLUG_UNPLUGGED          0   // 플러그 미연결
#define PLUG_CONNECTED_NO_PAY   1   // 플러그만 연결 (결제 전)
#define PLUG_CONNECTED_PAID     2   // 플러그 연결 + 결제 완료

// 고장 상태
#define FAULT_NORMAL    0       // 정상
#define FAULT_DETECT    1       // 고장 감지 (판정 중)
#define FAULT_CONFIRM   2       // 고장 확정 (조치 필요)

/* ============ Global Variables (전역 변수) ============ */

// === Sequence & Control Flags ===
int32_t SeqState = SEQ_INIT;                // 현재 시퀀스 상태
int32_t FLAG_Stop = 1;                      // 충전 중지 플래그 (1=정지, 0=진행)
int32_t FLAG_relay = 0;                     // 메인 릴레이 상태 (1=ON, 0=OFF)
int32_t ChargingTime = 0;                   // 누적 충전 시간 (사이클 카운트)

// === Input/Output Status ===
int32_t PlugInfo = PLUG_UNPLUGGED;          // 플러그 연결 상태
int32_t RelayInfo = 0;                      // 릴레이 실제 상태 피드백 (1=ON, 0=OFF)

// === Input AC Power Signals (입력 AC 전력 신호) ===
float Va = 0.0f;                            // A상 입력 전압 (V)
float Vb = 0.0f;                            // B상 입력 전압 (V)
float Vc = 0.0f;                            // C상 입력 전압 (V)
float Ia = 0.0f;                            // A상 입력 전류 (A)
float Ib = 0.0f;                            // B상 입력 전류 (A)
float Ic = 0.0f;                            // C상 입력 전류 (A)
float IsAvg = 0.0f;                         // 입력 전류 평균 (A) = (|Ia| + |Ib| + |Ic|) / 3

// === Fault Detection & Diagnosis ===
int32_t FaultState = FAULT_NORMAL;          // 고장 상태 (Normal/Detect/Confirm)
int32_t FaultCom = 0;                       // 종합 고장 코드 (0=정상, 1~N=고장 타입)

// === Ramp-up Control (충전 시작 시 부드러운 전류 상승) ===
int32_t RampupCounter = 0;                  // Ramp-up 카운터 (0~100)
#define RAMPUP_MAX      100                 // Ramp-up 완료 카운터 값
#define TARGET_CURRENT  30.0f               // 목표 충전 전류 (30A)


/* ============ Function Prototypes ============ */
int32_t Read_PlugStatus(void);              // 플러그 연결 상태 읽기
int32_t Read_PaymentStatus(void);           // 결제 상태 읽기
int32_t Read_RelayFeedback(void);           // 릴레이 피드백 읽기
void    Measure_ACPower(void);              // AC 전력 신호 측정
void    Calculate_AvgCurrent(void);         // 평균 전류 계산
void    Control_Relay(int32_t state);       // 릴레이 제어
void    Protection(void);                   // 보호 로직 (고장 감지)


/* ============ Main Sequence State Machine ============ */

/**
 * OBC_Sequence()
 * 
 * 충전기 제어 시퀀스 메인 상태머신
 * 각 상태에서 현재 입력값을 확인하고, 다음 상태로의 전이 조건을 판단합니다.
 * 
 * 정상 플로우:
 *   INIT -> WAIT (플러그 연결) -> CHARGING (결제 완료) -> 종료
 */
void OBC_Sequence(void)
{
    // === 매 사이클마다 센서값 갱신 ===
    PlugInfo = Read_PlugStatus();           // 플러그 상태 업데이트
    RelayInfo = Read_RelayFeedback();       // 릴레이 피드백 확인
    Measure_ACPower();                      // AC 전압/전류 측정
    Calculate_AvgCurrent();                 // 평균 전류 계산
    
    // === 고장 감지 로직 (모든 상태에서 실행) ===
    Protection();                           // 실시간 보호 함수 호출
    
    // === State-based Control ===
    switch(SeqState)
    {
        /* ===== STATE 0: SEQ_INIT (초기화 상태) ===== */
        case SEQ_INIT:
            // 초기 상태에서는 모든 제어신호를 OFF/초기값으로 설정
            FLAG_Stop = 1;                  // 충전 중지 상태 유지
            FLAG_relay = 0;                 // 메인 릴레이 OFF
            Control_Relay(0);               // 실제 릴레이 제어
            
            FaultState = FAULT_NORMAL;      // 고장 상태 초기화
            FaultCom = 0;                   // 고장 코드 클리어
            ChargingTime = 0;               // 충전 시간 초기화
            RampupCounter = 0;              // Ramp-up 카운터 초기화
            
            // === INIT -> WAIT 전이 조건 ===
            // 플러그가 연결되었는지 확인
            if (PlugInfo > PLUG_UNPLUGGED)  // 플러그 연결됨
            {
                printf("[INFO] Plug connected, moving to WAIT state\n");
                SeqState = SEQ_WAIT;
            }
            break;

        /* ===== STATE 1: SEQ_WAIT (대기 상태) ===== */
        case SEQ_WAIT:
            // 플러그는 연결되었지만 결제 대기 중
            FLAG_Stop = 1;                  // 아직 충전 중지
            FLAG_relay = 0;                 // 릴레이는 OFF 유지
            Control_Relay(0);
            
            // 플러그 상태 확인
            if (PlugInfo == PLUG_UNPLUGGED)  // 플러그가 빠짐
            {
                printf("[INFO] Plug disconnected, back to INIT\n");
                SeqState = SEQ_INIT;
            }
            // === WAIT -> CHARGING 전이 조건 ===
            // 결제 완료 && 고장 없음
            else if (PlugInfo == PLUG_CONNECTED_PAID && FaultState == FAULT_NORMAL)
            {
                printf("[INFO] Payment received, starting charging sequence\n");
                SeqState = SEQ_CHARGING;
                RampupCounter = 0;          // Ramp-up 준비
            }
            break;

        /* ===== STATE 2: SEQ_CHARGING (정상 충전 상태) ===== */
        case SEQ_CHARGING:
            // 정상 충전 중 - 충전 전류 점진적 상승 및 안정화
            FLAG_Stop = 0;                  // 충전 진행
            FLAG_relay = 1;                 // 메인 릴레이 ON
            Control_Relay(1);
            
            // === 전류 점진적 상승 (Ramp-up) ===
            // 초반에는 부하 충격을 피하기 위해 천천히 전류 증가
            if (RampupCounter < RAMPUP_MAX)
            {
                RampupCounter++;
                // 목표 전류까지 선형 증가하도록 리미터 설정
                // (실제로는 PID 제어나 소프트웨어 리미터가 담당)
                // IsAvg 값은 Measure_ACPower()에서 측정되어 자동 반영됨
            }
            
            // 충전 시간 누적
            ChargingTime++;
            
            // === 종료 조건 검사 ===
            if (PlugInfo == PLUG_UNPLUGGED)  // 플러그 제거
            {
                printf("[INFO] Plug disconnected during charging, stopping...\n");
                SeqState = SEQ_SHUTDOWN;
            }
            else if (ChargingTime > 3600)    // 충전 시간 초과 (예: 1시간 이상)
            {
                printf("[INFO] Charging time exceeded, charging complete\n");
                SeqState = SEQ_SHUTDOWN;
            }
            // === CHARGING -> FAULT 전이 ===
            // 고장 감지 시 자동으로 FAULT 상태로 전이
            // (Protection() 함수에서 FaultState 값 변경)
            else if (FaultState == FAULT_CONFIRM)
            {
                printf("[ERROR] Fault confirmed during charging, emergency stop!\n");
                SeqState = SEQ_FAULT;
            }
            break;

        /* ===== STATE 3: SEQ_FAULT (고장 감지 상태) ===== */
        case SEQ_FAULT:
            // 고장 감지됨 - 즉시 안전 조치 실행
            FLAG_Stop = 1;                  // 충전 강제 중지
            FLAG_relay = 0;                 // 릴레이 즉시 OFF
            Control_Relay(0);
            
            // 고장 코드 및 상태는 Protection() 함수에서 설정됨
            printf("[FAULT] FaultCom=0x%02X, FaultState=%d\n", FaultCom, FaultState);
            
            // 복구 가능성 판단 (고장 유형별로 다름)
            // 예: 과전류 잠시 발생 -> 회복 대기
            //     온도 과승 -> 냉각 필요
            //     통신 오류 -> 시스템 재부팅
            
            // 임시로 RESET 상태로 전이 (실제로는 고장 코드별 처리 필요)
            SeqState = SEQ_RESET;
            break;

        /* ===== STATE 4: SEQ_RESET (복구 시도 상태) ===== */
        case SEQ_RESET:
            // 고장 복구를 시도하거나, 사용자 개입 대기
            FLAG_Stop = 1;
            FLAG_relay = 0;
            
            // 고장이 여전한지 확인
            if (FaultState != FAULT_NORMAL)
            {
                printf("[INFO] Fault still present, maintaining RESET state\n");
                // RESET 상태 유지 (복구 대기)
            }
            else
            {
                printf("[INFO] Fault cleared, attempting to return to INIT\n");
                // 고장 클리어됨 -> 초기화 상태로 돌아가기
                SeqState = SEQ_INIT;
            }
            break;

        /* ===== STATE 5: SEQ_SHUTDOWN (종료 상태) ===== */
        case SEQ_SHUTDOWN:
            // 충전 종료 - 모든 릴레이/신호 OFF, 시스템 대기
            FLAG_Stop = 1;
            FLAG_relay = 0;
            Control_Relay(0);
            RampupCounter = 0;
            
            printf("[INFO] Charging completed/stopped. Ready for next cycle.\n");
            
            // 플러그가 빠질 때까지 SHUTDOWN 상태 유지
            if (PlugInfo == PLUG_UNPLUGGED)
            {
                SeqState = SEQ_INIT;
            }
            break;

        default:
            // 정의되지 않은 상태
            printf("[ERROR] Undefined state: %d\n", SeqState);
            SeqState = SEQ_INIT;
            break;
    }
}


/* ============ Utility Functions (보조 함수) ============ */

/**
 * Read_PlugStatus()
 * 플러그 연결 상태 읽기
 * 실제로는 GPIO/ADC 센서에서 읽음
 */
int32_t Read_PlugStatus(void)
{
    // TODO: 하드웨어에서 플러그 감지 센서 읽기
    // 예시: GPIO 핀에서 디지털 입력 -> PlugInfo 업데이트
    return PlugInfo;  // 현재는 전역 변수값 반환
}

/**
 * Read_PaymentStatus()
 * 결제 완료 상태 읽기
 * 실제로는 결제 시스템/서버에서 상태 수신
 */
int32_t Read_PaymentStatus(void)
{
    // TODO: 결제 게이트웨이/서버와 통신
    return (PlugInfo == PLUG_CONNECTED_PAID);
}

/**
 * Read_RelayFeedback()
 * 릴레이 제어 상태 피드백 읽기
 * 실제로는 릴레이 접점 상태 센서에서 읽음
 */
int32_t Read_RelayFeedback(void)
{
    // TODO: 릴레이 상태 센서 입력 읽기
    return RelayInfo;
}

/**
 * Measure_ACPower()
 * AC 입력 전압/전류 측정
 * 각 상(Phase A/B/C)의 전압과 전류를 ADC로 샘플링
 */
void Measure_ACPower(void)
{
    // TODO: ADC 드라이버로부터 신호 읽기
    // 예시 데이터 (실제로는 센서에서):
    // Va, Vb, Vc: 220V AC (±10% 변동)
    // Ia, Ib, Ic: 0~30A 범위 (RMS 값)
    
    // 현재 코드에서는 상태머신만 다루므로, 
    // 센서값 읽기는 하드웨어 레이어에서 자동 갱신된다고 가정
}

/**
 * Calculate_AvgCurrent()
 * 평균 전류 계산
 * 세 상의 절댓값 평균으로 계산
 */
void Calculate_AvgCurrent(void)
{
    // 입력 전류 평균값 계산 (RMS 기준)
    IsAvg = (fabs(Ia) + fabs(Ib) + fabs(Ic)) / 3.0f;
    
    // 디버그 출력 (선택)
    // printf("[CAN] IsAvg=%.2f A, Va=%.1f V\n", IsAvg, Va);
}

/**
 * Control_Relay(int32_t state)
 * 메인 릴레이 제어
 * state=1: 릴레이 ON (충전 경로 연결)
 * state=0: 릴레이 OFF (충전 경로 단절)
 */
void Control_Relay(int32_t state)
{
    // TODO: GPIO/PWM 드라이버로 릴레이 제어
    if (state == 1)
    {
        // 릴레이 ON 신호 (예: GPIO 핀 HIGH)
        printf("[RELAY] Main Relay ON\n");
        FLAG_relay = 1;
    }
    else
    {
        // 릴레이 OFF 신호 (예: GPIO 핀 LOW)
        printf("[RELAY] Main Relay OFF\n");
        FLAG_relay = 0;
    }
}

/**
 * Protection()
 * 보호 로직 및 고장 감지 (모든 상태에서 주기적으로 실행)
 * 
 * Week 1~2에서 학생들이 정의할 "고장 진단 조건"이 
 * 이 함수에 들어갈 예정입니다.
 */
void Protection(void)
{
    // === 정상 상태에서만 진단 시작 ===
    if (FaultState == FAULT_NORMAL)
    {
        // ===== Example 1: 입력 전류 과전류 고장 (IsAvg > 50A) =====
        if (IsAvg > 50.0f && SeqState == SEQ_CHARGING)
        {
            printf("[FAULT DETECT] Over-current detected! IsAvg=%.2f A\n", IsAvg);
            FaultCom = 0x01;            // 과전류 고장 코드
            FaultState = FAULT_CONFIRM; // 즉시 고장 확정 (심각한 고장)
        }
        
        // ===== Example 2: 입력 전압 과전압 고장 (Va > 250V) =====
        else if (Va > 250.0f && SeqState == SEQ_CHARGING)
        {
            printf("[FAULT DETECT] Over-voltage detected! Va=%.1f V\n", Va);
            FaultCom = 0x02;            // 과전압 고장 코드
            FaultState = FAULT_CONFIRM;
        }
        
        // ===== Example 3: 저전류 진단 (2사이클 연속 IsAvg < -5) =====
        // 일반적으로 저전류는 즉시 확정이 아니라, 누적 카운트로 확정
        // -> Week 2에서 학생들이 직접 설계할 영역
        else if (IsAvg < -5.0f && SeqState == SEQ_CHARGING)
        {
            printf("[FAULT DETECT] Under-current trend detected. IsAvg=%.2f A\n", IsAvg);
            FaultState = FAULT_DETECT;  // 진단 중 (카운트 누적 필요)
            FaultCom = 0x03;
        }
    }
    
    // === 고장 복구 로직 ===
    // (고장이 명확히 해결되었는지 재확인)
    // 예: 과전류 해제 시 고장 플래그 리셋
    if (FaultState == FAULT_DETECT && IsAvg < 50.0f && IsAvg > 0.0f)
    {
        printf("[FAULT RECOVERY] Condition normalized. Fault cleared.\n");
        FaultState = FAULT_NORMAL;
        FaultCom = 0x00;
    }
}


/* ============ Main Loop (메인 루프) ============ */

/**
 * main()
 * 
 * 교육용 시뮬레이션
 * 실제로는 이 루프가 하드웨어 타이머 인터럽트나 RTOS 태스크로 주기 실행됨
 */
int main(void)
{
    printf("=== OBC Sequence State Machine Simulation ===\n\n");
    
    // === 시뮬레이션 시나리오 ===
    for (int cycle = 0; cycle < 50; cycle++)
    {
        printf("\n[Cycle %3d] ", cycle);
        
        // 시뮬레이션 입력값 변경 (학생들이 이 부분으로 고장 시뮬레이션 할 수 있음)
        if (cycle == 0)         { PlugInfo = PLUG_CONNECTED_NO_PAY; }  // 플러그 연결
        if (cycle == 5)         { PlugInfo = PLUG_CONNECTED_PAID; }    // 결제 완료
        if (cycle == 10 && cycle < 15) { Va = 260.0f; }               // 과전압 시뮬레이션
        if (cycle == 20)        { IsAvg = 55.0f; }                     // 과전류 시뮬레이션
        if (cycle == 30)        { PlugInfo = PLUG_UNPLUGGED; }         // 플러그 제거
        
        // 주기 실행: 시퀀스 상태 업데이트
        OBC_Sequence();
        
        // 현재 상태 출력
        const char *seq_str[] = {"INIT", "WAIT", "CHARGING", "FAULT", "RESET", "SHUTDOWN"};
        printf("SeqState=%s, PlugInfo=%d, IsAvg=%.1fA, Relay=%d\n",
               seq_str[SeqState], PlugInfo, IsAvg, FLAG_relay);
    }
    
    printf("\n=== Simulation Complete ===\n");
    return 0;
}


/* ============ Notes for Students ============
 *
 * 이 코드를 보면서 다음을 주목하세요:
 *
 * 1. State Machine Structure:
 *    - 각 상태(INIT, WAIT, CHARGING, FAULT, ...)는 명확한 진입/진출 조건을 가집니다.
 *    - 시퀀스 전이는 센서값(PlugInfo, IsAvg, etc.)에 기반합니다.
 *
 * 2. CAN Signals in Context:
 *    - IsAvg, Va, Ia 같은 신호들은 각 사이클마다 갱신됩니다.
 *    - Protection() 함수에서 이 신호들을 이용해 고장을 감지합니다.
 *
 * 예시:
 *   if (IsAvg > 50.0f && SeqState == SEQ_CHARGING)
 *       -> 과전류 고장 진단
 *   if (FaultCnt > 20)
 *       -> 고장 확정
 *   if (IsAvg < 30.0f && FaultRecoveryCnt > 10)
 *       -> 고장 회복
 *
 *===========================================================================*/
