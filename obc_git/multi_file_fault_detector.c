#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// =============================================================
// LOGIC COPIED FROM 02_main.c 
// =============================================================

// --- 1. Constants & Macros ---
#define SEQ_INIT        0
#define SEQ_WAIT        1
#define SEQ_CHARGING    2
#define SEQ_FAULT       3
#define SEQ_RESET       4
#define SEQ_SHUTDOWN    5

// Thresholds
#define I_MAX           32.0f   
#define I_NORMAL_MAX    24.0f
#define I_MIN           6.0f    
#define I_NORMAL_MIN    12.0f
#define TEMP_MAX        60      
#define TEMP_REC        20
#define TEMP_MIN_VALID  -20
#define TEMP_MAX_VALID  120
#define ISO_R_MIN       500     
#define ISO_R_NORMAL    600
#define TIME_TH_10      10      
#define TIME_TH_05      5       
#define TIME_TH_03      3       

// Plug & Pay State
#define PLUG_INVALID    0
#define PLUG_VALID      1
#define PAY_NO_PAY      0
#define PAY_PAID        1

// --- 2. Data Structures ---
typedef struct {
    uint8_t current_sequence;       
    uint32_t seq_timer;             
    uint32_t full_charge_cnt_th;    
    
    float Ia, Ib, Ic;               
    int32_t real_batt_volt;         
    int32_t exp_batt_volt;          
    int32_t temperature;            
    
    uint32_t charge_cnt;            
    uint8_t plug_info;              
    uint8_t pay_status;             
    
    bool flag_relay;                
    bool flag_stop;                 
    
    uint8_t can_msg_received;       
    uint32_t wdt_unreceived_timer;  
    uint32_t iso_resistance;        
} SystemInput_t;

typedef struct {
    bool fault_active[13];
    bool system_permanent_lock;
    uint8_t over_cnt;
    uint8_t over_rec_cnt;
    uint8_t under_cnt;
    uint8_t under_rec_cnt;
    uint8_t fault_03_occur_cnt; 
    uint8_t batt_cnt;
    uint8_t batt_reset_cnt;
    uint8_t heat_cnt;
    uint8_t fault_06_occur_cnt; 
    uint8_t can_timeout;
    uint8_t can_recover;
    uint8_t iso_cnt;
    uint8_t iso_recover_cnt;
    uint8_t pay_err_cnt;
    uint8_t fault_0B_occur_cnt; 
    uint8_t temp_sensor_cnt;
} FaultState_t;

FaultState_t fault_state = {0}; 

// --- 3. Helper Functions ---
float Max3(float a, float b, float c) {
    float max = a > b ? a : b;
    return max > c ? max : c;
}
float Min3(float a, float b, float c) {
    float min = a < b ? a : b;
    return min < c ? min : c;
}

// --- 4. Main Fault Diagnosis Loop ---
void OBC_FaultDiagnosis_Loop(SystemInput_t *input) {
    
    if (fault_state.system_permanent_lock) {
        return; 
    }

    // 0x01: Over Current
    if (!fault_state.fault_active[1]) {
        if (input->current_sequence == SEQ_CHARGING && Max3(input->Ia, input->Ib, input->Ic) > I_MAX) {
            fault_state.over_cnt++;
            if (fault_state.over_cnt >= TIME_TH_10) {
                fault_state.fault_active[1] = true;
                fault_state.over_cnt = 0;
            }
        } else { fault_state.over_cnt = 0; }
    } else {
        if (Max3(input->Ia, input->Ib, input->Ic) < I_NORMAL_MAX) {
            fault_state.over_rec_cnt++;
            if (fault_state.over_rec_cnt >= TIME_TH_10) {
                fault_state.fault_active[1] = false;
                fault_state.over_rec_cnt = 0;
            }
        } else { fault_state.over_rec_cnt = 0; }
    }

    // 0x02: Under Current
    if (!fault_state.fault_active[2]) {
        if (input->current_sequence == SEQ_CHARGING && input->charge_cnt > 20 && 
            Min3(input->Ia, input->Ib, input->Ic) < I_MIN) {
            fault_state.under_cnt++;
            if (fault_state.under_cnt >= TIME_TH_10) {
                fault_state.fault_active[2] = true;
                fault_state.under_cnt = 0;
            }
        } else { fault_state.under_cnt = 0; }
    } else {
        if (Min3(input->Ia, input->Ib, input->Ic) > I_NORMAL_MIN) {
            fault_state.under_rec_cnt++;
            if (fault_state.under_rec_cnt >= TIME_TH_10) {
                fault_state.fault_active[2] = false;
                fault_state.under_rec_cnt = 0;
            }
        } else { fault_state.under_rec_cnt = 0; }
    }

    // 0x03: Plug Power Error
    if (!fault_state.fault_active[3]) {
        bool condition = (input->plug_info == PLUG_INVALID) || 
                         (input->plug_info == PLUG_VALID && Min3(input->Ia, input->Ib, input->Ic) < 0);
        
        if (input->current_sequence == SEQ_CHARGING && condition) {
            fault_state.fault_active[3] = true;
            fault_state.fault_03_occur_cnt++;
            if (fault_state.fault_03_occur_cnt >= 3) {
                fault_state.system_permanent_lock = true;
            }
        }
    } else {
        if (input->current_sequence != SEQ_CHARGING && input->plug_info == PLUG_VALID) {
            fault_state.fault_active[3] = false;
        }
    }

    // 0x04: Relay Error
    bool relay_fail_cond = (input->flag_relay == 1 && input->flag_stop == 1) || 
                           (input->flag_relay == 0 && input->flag_stop == 0);
    if (relay_fail_cond) {
        fault_state.fault_active[4] = true; 
    } else {
        fault_state.fault_active[4] = false;
    }

    // 0x05: BMS Error
    int32_t v_diff = abs(input->real_batt_volt - input->exp_batt_volt);
    if (!fault_state.fault_active[5]) {
        if (input->current_sequence == SEQ_CHARGING && input->charge_cnt > 10 && v_diff > 10) {
            fault_state.batt_cnt++;
            if (fault_state.batt_cnt >= TIME_TH_10) {
                fault_state.fault_active[5] = true;
                fault_state.batt_cnt = 0;
            }
        } else { fault_state.batt_cnt = 0; }
    } else {
        if (v_diff <= 5) {
            fault_state.batt_reset_cnt++;
            if (fault_state.batt_reset_cnt >= TIME_TH_10) {
                fault_state.fault_active[5] = false;
                fault_state.batt_reset_cnt = 0;
            }
        } else { fault_state.batt_reset_cnt = 0; }
    }

    // 0x06: Overheat
    if (!fault_state.fault_active[6]) {
        if (input->current_sequence == SEQ_CHARGING && input->temperature > TEMP_MAX) {
            fault_state.heat_cnt++;
            if (fault_state.heat_cnt >= TIME_TH_10) {
                fault_state.fault_active[6] = true;
                fault_state.heat_cnt = 0;
                fault_state.fault_06_occur_cnt++;
                if (fault_state.fault_06_occur_cnt >= 3) {
                    fault_state.system_permanent_lock = true;
                }
            }
        } else { fault_state.heat_cnt = 0; }
    } else {
        if (input->current_sequence == SEQ_INIT && input->temperature < TEMP_REC) {
            fault_state.fault_active[6] = false;
        }
    }

    // 0x07: CAN Error
    if (!fault_state.fault_active[7]) {
        if (input->can_msg_received == 0) {
            fault_state.can_timeout++;
            if (fault_state.can_timeout >= TIME_TH_05) {
                fault_state.fault_active[7] = true;
                fault_state.can_timeout = 0;
            }
        } else { fault_state.can_timeout = 0; }
    } else {
        if (input->can_msg_received == 1) {
            fault_state.can_recover++;
            if (fault_state.can_recover >= TIME_TH_05) {
                fault_state.fault_active[7] = false;
                fault_state.can_recover = 0;
            }
        } else { fault_state.can_recover = 0; }
    }

    // 0x08: Isolation Res Error
    if (!fault_state.fault_active[8]) {
        if (input->current_sequence == SEQ_CHARGING && input->iso_resistance < ISO_R_MIN) {
            fault_state.iso_cnt++;
            if (fault_state.iso_cnt >= TIME_TH_10) {
                fault_state.fault_active[8] = true;
                fault_state.iso_cnt = 0;
            }
        } else { fault_state.iso_cnt = 0; }
    } else {
        if (input->current_sequence != SEQ_CHARGING && input->iso_resistance > ISO_R_NORMAL) {
            fault_state.iso_recover_cnt++;
            if (fault_state.iso_recover_cnt >= TIME_TH_10) {
                fault_state.fault_active[8] = false;
                fault_state.iso_recover_cnt = 0;
            }
        } else { fault_state.iso_recover_cnt = 0; }
    }

    // 0x09: Payment Error
    if (!fault_state.fault_active[9]) {
        if (input->plug_info == PLUG_VALID && input->pay_status == PAY_NO_PAY) {
            fault_state.pay_err_cnt++;
            if (fault_state.pay_err_cnt >= TIME_TH_05) {
                fault_state.fault_active[9] = true;
                fault_state.pay_err_cnt = 0;
            }
        } else { fault_state.pay_err_cnt = 0; }
    } else {
        bool rec_cond1 = (input->plug_info == PLUG_VALID && input->pay_status == PAY_PAID && input->current_sequence == SEQ_CHARGING);
        bool rec_cond2 = (input->current_sequence == SEQ_INIT);
        if (rec_cond1 || rec_cond2) {
            fault_state.fault_active[9] = false;
        }
    }

    // 0x0A: WDT
    if (input->wdt_unreceived_timer >= 10) {
        fault_state.fault_active[10] = true; 
        fault_state.system_permanent_lock = true; 
    }

    // 0x0B: Timeout
    bool seq_timeout_cond = false;
    if ((input->current_sequence == SEQ_INIT || input->current_sequence == SEQ_WAIT) && input->seq_timer >= 10) {
        seq_timeout_cond = true;
    } else if (input->current_sequence == SEQ_CHARGING && input->seq_timer > input->full_charge_cnt_th) {
        seq_timeout_cond = true;
    } else if ((input->current_sequence == SEQ_FAULT || input->current_sequence == SEQ_RESET) && input->seq_timer >= 10) {
        seq_timeout_cond = true;
    }
    if (!fault_state.fault_active[11]) {
        if (seq_timeout_cond) {
            fault_state.fault_active[11] = true;
            fault_state.fault_0B_occur_cnt++;
            if (fault_state.fault_0B_occur_cnt >= 3) {
                fault_state.system_permanent_lock = true;
            }
        }
    } else {
        if (input->current_sequence == SEQ_INIT) {
            fault_state.fault_active[11] = false;
        }
    }

    // 0x0C: Temp Sensor
    if (!fault_state.fault_active[12]) {
        bool temp_invalid = (input->temperature < TEMP_MIN_VALID) || (input->temperature > TEMP_MAX_VALID);
        if (input->current_sequence == SEQ_CHARGING && temp_invalid) {
            fault_state.temp_sensor_cnt++;
            if (fault_state.temp_sensor_cnt >= TIME_TH_03) {
                fault_state.fault_active[12] = true;
                fault_state.temp_sensor_cnt = 0;
            }
        } else { fault_state.temp_sensor_cnt = 0; }
    } else {
        bool temp_valid = (input->temperature >= TEMP_MIN_VALID) && (input->temperature <= TEMP_MAX_VALID);
        if (input->current_sequence == SEQ_INIT && temp_valid) {
            fault_state.fault_active[12] = false;
            fault_state.temp_sensor_cnt = 0;
        }
    }
}

// =============================================================
// MAIN APP
// =============================================================

#define MAX_LINE_LENGTH 2048

// Removed Korean characters to prevent encoding issues as requested
const char* get_fault_name(int fault_code) {
    switch(fault_code) {
        case 0x01: return "Over Current";
        case 0x02: return "Under Current";
        case 0x03: return "Plug Power Error";
        case 0x04: return "Relay Error";
        case 0x05: return "BMS Error";
        case 0x06: return "Overheat";
        case 0x07: return "CAN Communication Error";
        case 0x08: return "Isolation Resistance Error";
        case 0x09: return "Payment Error";
        case 0x0A: return "WDT Error";
        case 0x0B: return "Sequence Timeout";
        case 0x0C: return "Temp Sensor Error";
        default: return "Unknown";
    }
}

const char* get_csv_value(char *line, int target_index) {
    static char buffer[100];
    char *line_copy = strdup(line);
    char *token = strtok(line_copy, ",");
    int current_index = 0;
    while (token != NULL) {
        if (current_index == target_index) {
            strncpy(buffer, token, 99);
            buffer[99] = '\0';
            free(line_copy);
            return buffer;
        }
        token = strtok(NULL, ",");
        current_index++;
    }
    free(line_copy);
    return "0"; 
}

void reset_fault_state() {
    memset(&fault_state, 0, sizeof(FaultState_t));
}

void process_file(const char *filename) {
    printf("\n>>> Analyzing File: %s\n", filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Could not open file '%s'.\n", filename);
        return;
    }

    reset_fault_state();
    char line[MAX_LINE_LENGTH];
    
    // Read Header
    char *header = fgets(line, MAX_LINE_LENGTH, fp);
    if (header == NULL) {
        printf("Error: Empty file.\n");
        fclose(fp);
        return;
    }
    
    // Keep header string for later if needed (we won't print it, just useful context)
    // char header_copy[MAX_LINE_LENGTH];
    // strcpy(header_copy, header);

    // Column Mapping
    int col_seq = 1;
    int col_plug = 2;
    int col_stop = 3;
    int col_relay = 4;
    int col_Ia = 5;
    int col_Ib = 6;
    int col_Ic = 7;
    int col_chg_cnt = 9;
    int col_volt_real = 10;
    int col_volt_exp = 11;
    int col_temp = 12; 
    int col_can = 13;
    int col_iso = 14;

    SystemInput_t input = {0};
    int row_count = 0;
    
    bool fault_previously_active[13] = {false};
    bool detected_any_permanent = false;

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        row_count++;
        
        // Remove trailing newline for cleaner printing
        line[strcspn(line, "\r\n")] = 0;

        // Parse Data
        input.current_sequence = atoi(get_csv_value(line, col_seq));
        int raw_plug_info = atoi(get_csv_value(line, col_plug));

        // Logic Mapping
        if (raw_plug_info == 0) {
            input.plug_info = PLUG_INVALID; // 0
            input.pay_status = PAY_NO_PAY;  // 0
        } else if (raw_plug_info == 1) {
            input.plug_info = PLUG_VALID;   // 1
            input.pay_status = PAY_NO_PAY;  // 0
        } else if (raw_plug_info == 2) {
            input.plug_info = PLUG_VALID;   // 1
            input.pay_status = PAY_PAID;    // 1
        } else {
            input.plug_info = PLUG_VALID;
            input.pay_status = PAY_PAID;
        }

        input.flag_stop = atoi(get_csv_value(line, col_stop));
        input.flag_relay = atoi(get_csv_value(line, col_relay));
        
        input.Ia = atof(get_csv_value(line, col_Ia));
        input.Ib = atof(get_csv_value(line, col_Ib));
        input.Ic = atof(get_csv_value(line, col_Ic));
        
        input.charge_cnt = atoi(get_csv_value(line, col_chg_cnt)); 
        input.real_batt_volt = atoi(get_csv_value(line, col_volt_real));
        input.exp_batt_volt = atoi(get_csv_value(line, col_volt_exp));
        
        input.temperature = atoi(get_csv_value(line, col_temp));
        input.can_msg_received = atoi(get_csv_value(line, col_can));
        input.iso_resistance = atoi(get_csv_value(line, col_iso));
        
        input.seq_timer = 0; 
        
        // Run Diagnosis
        OBC_FaultDiagnosis_Loop(&input);
        
        // Check Status Changes
        for (int i = 1; i <= 12; i++) {
             if (fault_state.fault_active[i] && !fault_previously_active[i]) {
                 printf("\n  [DETECTED] Code 0x%02X: %s\n", i, get_fault_name(i));
                 printf("             at Row %d\n", row_count);
                 printf("             Data: %s\n", line); // Print FULL ROW data
                 fault_previously_active[i] = true;
                 detected_any_permanent = true;
             } 
             else if (!fault_state.fault_active[i] && fault_previously_active[i]) {
                 printf("\n  [CLEARED]  Code 0x%02X: %s\n", i, get_fault_name(i));
                 printf("             at Row %d\n", row_count);
                 printf("             Data: %s\n", line); // Print FULL ROW data
                 fault_previously_active[i] = false;
             }
        }
        
        if (fault_state.system_permanent_lock) {
            printf("\n  [LOCKED] System Permanent Lock triggered at Row %d!\n", row_count);
            printf("           Data: %s\n", line);
            break; 
        }
    }
    
    if (!detected_any_permanent) {
        printf("  [RESULT] No faults detected (Passed).\n");
    }

    fclose(fp);
}

int main() {
    printf("=== Multi-File Fault Detection ===\n");
    const char *files[] = {
        "Fault1.csv", 
        "Fault2.csv", 
        "Fault3.csv", 
        "Fault4.csv"
    };
    int num_files = 4;

    for (int i = 0; i < num_files; i++) {
        process_file(files[i]);
    }
    printf("\n=== Analysis Complete ===\n");
    return 0;
}
