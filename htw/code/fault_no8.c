#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define TARGET_COLUMN_INDEX 14 // IsoR은 15번째 열이므로 인덱스 14

// 진단 관련 상수 정의
#define THRESHOLD_FAULT 10   // 고장 확정 누적 횟수
#define THRESHOLD_RECOVER 10 // 고장 회복 누적 횟수
#define ISOR_MIN_FAULT 500000 // 500k 이하 고장
#define ISOR_NORMAL_RECOVER 600000 // 600k 이상 정상

void detect_fault(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return;
    }

    char line[MAX_LINE_LENGTH];
    int row_count = 0;
    
    // 로직 변수 초기화
    int iso_fault_count = 0;
    int iso_recover_count = 0;
    int fault_active = 0; // 0: 정상, 1: 고장 중

    // 헤더 건너뛰기
    if (fgets(line, MAX_LINE_LENGTH, file) == NULL) {
        printf("파일이 비어있습니다.\n");
        fclose(file);
        return;
    }

    printf("=== Fault 8 (절연저항) 진단 시작: %s ===\n", filename);

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        row_count++; 
        
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, line);

        char *token = strtok(line_copy, ",");
        int current_col = 0;
        int current_isor = -1;

        while (token != NULL) {
            if (current_col == TARGET_COLUMN_INDEX) {
                // 빈 값 체크 (연속된 쉼표 등)
                if (strlen(token) > 0) {
                     current_isor = atoi(token);
                }
                break;
            }
            token = strtok(NULL, ",");
            current_col++;
        }

        if (current_isor == -1) {
            // 값이 없거나 파싱 실패 시 건너뜀 (CSV 마지막 줄 등)
            continue; 
        }

        // === [핵심 로직 구현] ===
        
        if (current_isor <= ISOR_MIN_FAULT) {
            // 진단 조건: IsoR <= 500k (누적)
            iso_fault_count++;
        } else if (current_isor >= ISOR_NORMAL_RECOVER) {
            // 회복 조건: IsoR >= 600k (누적)
            iso_recover_count++;
        }
        // *참고*: 500k 초과 600k 미만인 구간은 아무 카운트도 올리지 않음 (기존 카운트 유지)

        // 고장 확정 조건 체크 (정상 상태일 때만)
        if (fault_active == 0 && iso_fault_count >= THRESHOLD_FAULT) {
            fault_active = 1;
            iso_recover_count = 0; // 고장 확정 시 회복 카운트 리셋
            printf("[고장 발생] Row %d : 절연저항 이상 확정 (IsoR: %d, Count: %d)\n", row_count, current_isor, iso_fault_count);
        }

        // 고장 회복 조건 체크 (고장 상태일 때만)
        else if (fault_active == 1 && iso_recover_count >= THRESHOLD_RECOVER) {
            fault_active = 0;
            iso_fault_count = 0; // 회복 확정 시 고장 카운트 리셋
            printf("[고장 해제] Row %d : 절연저항 정상 복귀 (IsoR: %d, Recover Count: %d)\n", row_count, current_isor, iso_recover_count);
        }
    }

    fclose(file);
    printf("=== 진단 종료 ===\n");
}

int main() {
    detect_fault("fault_no7.csv"); 
    return 0;
}
