#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define TARGET_COLUMN_INDEX 13 // 0부터 시작하므로 14번째 열은 인덱스 13

// 진단 관련 상수 정의
#define THRESHOLD_TIMEOUT 5
#define THRESHOLD_RECOVER 5

void detect_fault(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        printf("파일 이름이 정확한지, 같은 폴더에 있는지 확인해주세요.\n");
        return;
    }

    char line[MAX_LINE_LENGTH];
    int row_count = 0;
    
    // 로직 변수 초기화
    int can_timeout = 0;
    int can_recover = 0;
    int fault_active = 0; // 0: 정상, 1: 고장 중

    // 헤더(첫 줄) 건너뛰기
    if (fgets(line, MAX_LINE_LENGTH, file) == NULL) {
        printf("파일이 비어있습니다.\n");
        fclose(file);
        return;
    }

    printf("=== 진단 시작: %s ===\n", filename);

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        row_count++; // 데이터 행 번호 (1부터 시작)
        
        // CSV 파싱을 위해 라인 복사 (strtok은 원본을 변경하므로)
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, line);

        // 14번째 열(CanMsg_Received) 추출
        char *token = strtok(line_copy, ",");
        int current_col = 0;
        int can_msg_received = -1;

        while (token != NULL) {
            if (current_col == TARGET_COLUMN_INDEX) {
                can_msg_received = atoi(token);
                break;
            }
            token = strtok(NULL, ",");
            current_col++;
        }

        if (can_msg_received == -1) {
            // 해당 열을 찾지 못한 경우 (데이터 오류 등)
            continue; 
        }

        // === [핵심 로직 구현] ===
        
        if (can_msg_received == 0) {
            // 진단 조건: 메시지 수신 실패 (누적)
            can_timeout++;
        } else {
            // 회복 조건: 메시지 수신 성공 (누적)
            can_recover++;
        }

        // 고장 확정 조건 체크 (정상 상태일 때만 체크)
        if (fault_active == 0 && can_timeout >= THRESHOLD_TIMEOUT) {
            fault_active = 1;
            can_recover = 0; // *중요* 고장 확정 시점부터 회복 카운트 새로 시작
            printf("[고장 발생] Row %d : CAN 통신 오류 확정 (TimeOut Count: %d)\n", row_count, can_timeout);
        }

        // 고장 회복 조건 체크 (고장 상태일 때만 체크)
        else if (fault_active == 1 && can_recover >= THRESHOLD_RECOVER) {
            fault_active = 0;
            can_timeout = 0; // *중요* 회복 확정 시점부터 고장 카운트 새로 시작
            printf("[고장 해제] Row %d : CAN 통신 정상 복귀 (Recover Count: %d)\n", row_count, can_recover);
        }

        // (선택 사항) 고장이 지속되는 모든 행을 보고싶다면 아래 주석 해제
        /*
        if (fault_active) {
            printf("Row %d: 고장 상태 지속 중...\n", row_count);
        }
        */
    }

    fclose(file);
    printf("=== 진단 종료 ===\n");
}

int main() {
    // 분석할 파일명 (업로드하신 파일명에 맞춰 수정 가능)
    // 엑셀(.xlsx) 파일을 '다른 이름으로 저장'하여 .csv로 변환 후 실행해야 합니다.
    detect_fault("fault_no7.csv"); 
    return 0;
}