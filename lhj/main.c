#include <stdio.h>
#include "fault.h"

/* ============================================================================
 *  Program : fault_engine.exe
 *  Purpose : OBC Fault Diagnostic Engine
 *
 *  Usage   : fault_engine.exe input.csv result.csv
 *
 *  Design  :
 *   - argv[1] : Input CSV file path
 *   - argv[2] : Result CSV file path
 *   - argv[0] : Executable name (NOT used in logic)
 *
 * ============================================================================ */

int main(int argc, char* argv[])
{
    /* ------------------------------
     * Argument validation
     * ------------------------------ */
    if (argc < 3)
    {
        printf("ERROR: 입력 파일과 결과 파일이 입력되지 않았습니다.\n");
        return 1;
    }

    /* ------------------------------
     * File path binding
     * ------------------------------ */
	const char* exe_name = argv[0];
    const char* input_file = argv[1];
    const char* result_file = argv[2];

	printf("exe File   : %s\n", exe_name);
	printf("Input File   : %s\n", input_file);
	printf("Result File  : %s\n", result_file);


    /* ------------------------------
     * File open
     * ------------------------------ */
    FILE* fp = fopen(input_file, "r");
    FILE* out = fopen(result_file, "w");

    InputSnapshot in = { 0 };

    if (!fp)
    {
        printf("ERROR: Failed to open input CSV : %s\n", input_file);
        return 1;
    }

    if (!out)
    {
        printf("ERROR: Failed to open result CSV : %s\n", result_file);
        fclose(fp);
        return 1;
    }

    /* ------------------------------
     * CSV Header
     * ------------------------------ */
    fprintf(out,
        "Cycle,"
        "F_0x01,F_0x02,F_0x03,F_0x04,F_0x05,F_0x06,"
        "F_0x07,F_0x08,F_0x09,F_0x0A,F_0x0B,F_0x0C\n"
    );

    /* ------------------------------
     * Fault system initialization
     * ------------------------------ */
    Fault_Init();

    /* ------------------------------
     * Main diagnostic loop
     * ------------------------------ */
	Input_ReadLine(fp, &in); // 헤더 스킵

    while (Input_ReadLine(fp, &in))
    {
        /* 1. Fault diagnosis (decision only) */
        Fault_Diagnose(&in);

        /* 2. Write result */
        fprintf(out, "%d", in.Cycle);

        fprintf(out, ",%d", Fault_GetStatus(FAULT_INPUT_OVERCURRENT));   // 0x01
        fprintf(out, ",%d", Fault_GetStatus(FAULT_INPUT_UNDERCURRENT));  // 0x02
        fprintf(out, ",%d", Fault_GetStatus(FAULT_PLUG));                // 0x03
        fprintf(out, ",%d", Fault_GetStatus(FAULT_RELAY));               // 0x04
        fprintf(out, ",%d", Fault_GetStatus(FAULT_BMS_STATE));           // 0x05
        fprintf(out, ",%d", Fault_GetStatus(FAULT_OVER_TEMP));           // 0x06
        fprintf(out, ",%d", Fault_GetStatus(FAULT_CAN));                 // 0x07
        fprintf(out, ",%d", Fault_GetStatus(FAULT_ISO));                 // 0x08
        fprintf(out, ",%d", Fault_GetStatus(FAULT_PAYMENT));             // 0x09
        fprintf(out, ",%d", Fault_GetStatus(FAULT_WDT));                 // 0x0A
        fprintf(out, ",%d", Fault_GetStatus(FAULT_SEQ_TIMEOUT));         // 0x0B
        fprintf(out, ",%d", Fault_GetStatus(FAULT_TEMP_SENSOR));         // 0x0C

        fprintf(out, "\n");
    }

    /* ------------------------------
     * Cleanup
     * ------------------------------ */
    fclose(fp);
    fclose(out);

    printf("Fault diagnosis completed successfully.\n");

    // 단위 테스트 코드입니다. 
    // 하나씩 실행 시키면, 각각의 test_case에 대한 결과값이 출력됩니다.
    /*Test_Fault_0x01("Unit_Test/fault_0x01_test.csv");*/
    /*Test_Fault_0x02("Unit_Test/fault_0x02_test.csv");*/
    /*Test_Fault_0x03("Unit_Test/fault_0x03_test.csv");*/
    /*Test_Fault_0x04("Unit_Test/fault_0x04_test.csv");*/
    /*Test_Fault_0x05("Unit_Test/fault_0x05_test.csv");*/
    /*Test_Fault_0x06("Unit_Test/fault_0x06_test.csv");*/
    /*Test_Fault_0x07("Unit_Test/fault_0x07_test.csv");*/
    /*Test_Fault_0x08("Unit_Test/fault_0x08_test.csv");*/
    /*Test_Fault_0x09("Unit_Test/fault_0x09_test.csv");*/
    /*Test_Fault_0x0A("Unit_Test/fault_0x0A_test.csv");*/
    /*Test_Fault_0x0B("Unit_Test/fault_0x0B_test.csv");*/
    /*Test_Fault_0x0C("Unit_Test/fault_0x0C_test.csv");*/
    return 0;
}
