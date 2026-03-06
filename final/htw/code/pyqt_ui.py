# -*- coding: utf-8 -*-
import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from PyQt5.QtWidgets import (
    QApplication, QWidget, QPushButton, QLabel,
    QFileDialog, QVBoxLayout, QHBoxLayout, QMessageBox,
    QListWidget, QListWidgetItem, QAbstractItemView, QDialog
)
from PyQt5.QtCore import QProcess, Qt


class FaultDiagUI(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("OBC Fault Diagnostic Tool")

        self.input_paths = []  # 다중 입력 파일 경로
        self.result_paths = []  # 다중 결과 파일 경로

        # UI 구성
        self.label = QLabel("Input CSV Files: Not selected")
        self.btn_select = QPushButton("Select Input CSV (Multiple)")
        self.btn_clear = QPushButton("Clear Selection")
        
        # 파일 목록 리스트
        self.file_list = QListWidget()
        self.file_list.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.file_list.setMinimumHeight(150)
        
        self.btn_run = QPushButton("Run Diagnosis (Selected)")
        self.btn_visualize = QPushButton("Visualize Results")

        # 버튼 연결
        self.btn_select.clicked.connect(self.select_csv)
        self.btn_clear.clicked.connect(self.clear_selection)
        self.btn_run.clicked.connect(self.run_diagnosis)
        self.btn_visualize.clicked.connect(self.visualize_results)

        # 레이아웃 구성
        layout = QVBoxLayout()
        layout.addWidget(self.label)
        layout.addWidget(self.btn_select)
        layout.addWidget(self.file_list)
        layout.addWidget(self.btn_clear)
        layout.addWidget(self.btn_run)
        layout.addWidget(self.btn_visualize)
        self.setLayout(layout)
        self.resize(1000, 600)

    # =========================
    # Select CSV (Multiple)
    # =========================
    def select_csv(self):
        paths, _ = QFileDialog.getOpenFileNames(
            self,
            "Select Input CSV Files",
            "",
            "CSV Files (*.csv)"
        )

        if paths:
            for path in paths:
                norm_path = os.path.normpath(path)
                if norm_path not in self.input_paths:
                    self.input_paths.append(norm_path)
                    item = QListWidgetItem(os.path.basename(norm_path))
                    item.setData(Qt.UserRole, norm_path)
                    self.file_list.addItem(item)
            
            self.label.setText(f"Input CSV Files: {len(self.input_paths)} files selected")

    # =========================
    # Clear Selection
    # =========================
    def clear_selection(self):
        self.input_paths = []
        self.result_paths = []
        self.file_list.clear()
        self.label.setText("Input CSV Files: Not selected")

    # =========================
    # Run diagnosis (EXE call)
    # =========================
    def run_diagnosis(self):
        selected_items = self.file_list.selectedItems()
        
        if not selected_items:
            QMessageBox.warning(
                self,
                "Error",
                "Please select files from the list to diagnose.\n"
                "(Use Ctrl+Click for multiple selection)"
            )
            return

        BASE_DIR = os.path.dirname(os.path.abspath(__file__))
        result_dir = os.path.join(BASE_DIR, "result")
        os.makedirs(result_dir, exist_ok=True)

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

        # 진단 실행
        self.result_paths = []
        success_count = 0
        fail_count = 0

        for item in selected_items:
            input_path = item.data(Qt.UserRole)
            base = os.path.splitext(os.path.basename(input_path))[0]
            result_path = os.path.normpath(
                os.path.join(result_dir, f"{base}_result.csv")
            )

            print("====================================")
            print("EXE PATH   :", exe_path)
            print("INPUT PATH :", input_path)
            print("RESULT PATH:", result_path)
            print("====================================")

            self.process = QProcess(self)
            self.process.setWorkingDirectory(os.path.join(BASE_DIR, "Debug"))
            self.process.readyReadStandardOutput.connect(
                lambda: print(self.process.readAllStandardOutput().data().decode())
            )
            self.process.readyReadStandardError.connect(
                lambda: print(self.process.readAllStandardError().data().decode())
            )

            self.process.start(exe_path, [input_path, result_path])
            self.process.waitForFinished()

            exit_code = self.process.exitCode()
            print("PROCESS EXIT CODE:", exit_code)

            if exit_code == 0:
                self.result_paths.append(result_path)
                success_count += 1
            else:
                fail_count += 1

        # 결과 메시지
        if success_count > 0:
            msg = f"Diagnosis completed.\n\nSuccess: {success_count} files"
            if fail_count > 0:
                msg += f"\nFailed: {fail_count} files"
            QMessageBox.information(self, "Completed", msg)
        else:
            QMessageBox.critical(self, "Error", "All diagnoses failed.")

    # =========================
    # Visualize Results
    # =========================
    def visualize_results(self):
        if not self.result_paths:
            # 결과 파일이 없으면 직접 선택
            paths, _ = QFileDialog.getOpenFileNames(
                self,
                "Select Result CSV Files",
                os.path.join(os.path.dirname(os.path.abspath(__file__)), "result"),
                "CSV Files (*.csv)"
            )
            if not paths:
                return
            self.result_paths = [os.path.normpath(p) for p in paths]

        # 시각화 다이얼로그 열기
        dialog = VisualizationDialog(self.result_paths, self)
        dialog.exec_()


# =========================
# Visualization Dialog (Paginated)
# =========================
class VisualizationDialog(QDialog):
    def __init__(self, result_paths, parent=None):
        super().__init__(parent)
        self.result_paths = result_paths
        self.current_index = 0

        self.setWindowTitle("Fault Diagnosis Visualization")
        self.resize(900, 600)

        # 레이아웃 구성
        layout = QVBoxLayout()

        # 파일명 라벨
        self.file_label = QLabel()
        self.file_label.setAlignment(Qt.AlignCenter)
        self.file_label.setStyleSheet("font-size: 14px; font-weight: bold; padding: 10px;")
        layout.addWidget(self.file_label)

        # matplotlib 캔버스
        self.figure = Figure(figsize=(10, 5))
        self.canvas = FigureCanvas(self.figure)
        layout.addWidget(self.canvas)

        # 네비게이션 버튼
        nav_layout = QHBoxLayout()
        self.btn_prev = QPushButton("◀ Prev")
        self.btn_next = QPushButton("Next ▶")
        self.page_label = QLabel()
        self.page_label.setAlignment(Qt.AlignCenter)

        self.btn_prev.clicked.connect(self.prev_page)
        self.btn_next.clicked.connect(self.next_page)

        nav_layout.addWidget(self.btn_prev)
        nav_layout.addWidget(self.page_label)
        nav_layout.addWidget(self.btn_next)
        layout.addLayout(nav_layout)

        self.setLayout(layout)

        # 첫 페이지 표시
        self.update_plot()

    def update_plot(self):
        self.figure.clear()
        
        if not self.result_paths:
            return

        result_path = self.result_paths[self.current_index]
        filename = os.path.basename(result_path)

        # 라벨 업데이트
        self.file_label.setText(f"File: {filename}")
        self.page_label.setText(f"{self.current_index + 1} / {len(self.result_paths)}")

        # 버튼 활성화/비활성화
        self.btn_prev.setEnabled(self.current_index > 0)
        self.btn_next.setEnabled(self.current_index < len(self.result_paths) - 1)

        # 데이터 읽기
        try:
            df = pd.read_csv(result_path)
        except Exception as e:
            ax = self.figure.add_subplot(111)
            ax.text(0.5, 0.5, f"Error loading file:\n{e}", ha='center', va='center')
            self.canvas.draw()
            return

        # 고장 코드 컬럼 추출
        fault_cols = [col for col in df.columns if col.startswith('F_0x')]

        if not fault_cols:
            ax = self.figure.add_subplot(111)
            ax.text(0.5, 0.5, "No fault columns found", ha='center', va='center')
            self.canvas.draw()
            return

        # 진단(1) 및 확정(2) 횟수 계산
        diagnosed_counts = [(df[col] == 1).sum() for col in fault_cols]
        confirmed_counts = [(df[col] == 2).sum() for col in fault_cols]

        # 차트 그리기
        ax = self.figure.add_subplot(111)
        x = range(len(fault_cols))
        width = 0.35

        bars1 = ax.bar([i - width/2 for i in x], diagnosed_counts, width, 
                       label='Diagnosed (1)', color='#FFA500')
        bars2 = ax.bar([i + width/2 for i in x], confirmed_counts, width, 
                       label='Confirmed (2)', color='#FF4444')

        ax.set_xlabel('Fault Code')
        ax.set_ylabel('Frequency')
        ax.set_title(f'Fault Diagnosis Results - {filename}')
        ax.set_xticks(x)
        ax.set_xticklabels(fault_cols, rotation=45, ha='right')
        ax.legend()

        # 막대 위에 숫자 표시
        for bar in bars1:
            height = bar.get_height()
            if height > 0:
                ax.annotate(f'{int(height)}',
                            xy=(bar.get_x() + bar.get_width() / 2, height),
                            ha='center', va='bottom', fontsize=9)
        for bar in bars2:
            height = bar.get_height()
            if height > 0:
                ax.annotate(f'{int(height)}',
                            xy=(bar.get_x() + bar.get_width() / 2, height),
                            ha='center', va='bottom', fontsize=9)

        self.figure.tight_layout()
        self.canvas.draw()

    def prev_page(self):
        if self.current_index > 0:
            self.current_index -= 1
            self.update_plot()

    def next_page(self):
        if self.current_index < len(self.result_paths) - 1:
            self.current_index += 1
            self.update_plot()


# =========================
# main
# =========================
if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = FaultDiagUI()
    win.show()
    sys.exit(app.exec_())
