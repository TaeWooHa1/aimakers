import sys
import os
from PyQt5.QtWidgets import (
    QApplication, QWidget, QPushButton, QLabel,
    QFileDialog, QVBoxLayout, QMessageBox
)
from PyQt5.QtCore import QProcess


class FaultDiagUI(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("OBC Fault Diagnostic Tool")
        self.resize(500, 230)

        self.input_path = ""

        # UI
        self.label = QLabel("Input CSV: Not selected")
        self.btn_select = QPushButton("Select Input CSV")
        self.btn_run = QPushButton("Run Diagnosis")

        self.btn_select.clicked.connect(self.select_csv)
        self.btn_run.clicked.connect(self.run_diagnosis)

        layout = QVBoxLayout()
        layout.addWidget(self.label)
        layout.addWidget(self.btn_select)
        layout.addWidget(self.btn_run)
        self.setLayout(layout)

    # =========================
    # Select CSV
    # =========================
    def select_csv(self):
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Select Input CSV",
            "",
            "CSV Files (*.csv)"
        )

        if path:
            self.input_path = os.path.normpath(path)
            self.label.setText(f"Input CSV:\n{self.input_path}")

    # =========================
    # Run diagnosis (EXE call)
    # =========================
    def run_diagnosis(self):
        if not self.input_path:
            QMessageBox.warning(
                self,
                "Error",
                "Please select an input CSV file first."
            )
            return

        BASE_DIR = os.path.dirname(os.path.abspath(__file__))

        # -------------------------
        # Ensure result directory
        # -------------------------
        result_dir = os.path.join(BASE_DIR, "result")
        os.makedirs(result_dir, exist_ok=True)

        # -------------------------
        # Result file path (lhj/result)
        # -------------------------
        base = os.path.splitext(os.path.basename(self.input_path))[0]
        result_path = os.path.normpath(
            os.path.join(result_dir, f"{base}_result.csv")
        )

        # -------------------------
        # EXE path (lhj/Debug)
        # -------------------------
        exe_path = os.path.normpath(
            os.path.join(BASE_DIR, "Debug", "OBC_FAULT_LOGIC.exe")
        )

        if not os.path.exists(exe_path):
            QMessageBox.critical(
                self,
                "Error",
                f"Cannot find OBC_FAULT_LOGIC.exe.\n\n{exe_path}"
            )
            return

        # -------------------------
        # Debug log
        # -------------------------
        print("====================================")
        print("EXE PATH   :", exe_path)
        print("INPUT PATH :", self.input_path)
        print("RESULT PATH:", result_path)
        print("====================================")

        # -------------------------
        # Run QProcess
        # -------------------------
        self.process = QProcess(self)

        # Set working directory to Debug
        self.process.setWorkingDirectory(
            os.path.join(BASE_DIR, "Debug")
        )

        self.process.readyReadStandardOutput.connect(
            lambda: print(self.process.readAllStandardOutput().data().decode())
        )
        self.process.readyReadStandardError.connect(
            lambda: print(self.process.readAllStandardError().data().decode())
        )

        self.process.start(exe_path, [self.input_path, result_path])
        self.process.waitForFinished()

        exit_code = self.process.exitCode()
        print("PROCESS EXIT CODE:", exit_code)

        if exit_code != 0:
            QMessageBox.critical(
                self,
                "Failure",
                "An error occurred during diagnosis.\n"
                "Please check the console log."
            )
            return

        QMessageBox.information(
            self,
            "Completed",
            f"Diagnosis completed successfully.\n\nResult file:\n{result_path}"
        )


# =========================
# main
# =========================
if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = FaultDiagUI()
    win.show()
    sys.exit(app.exec_())
