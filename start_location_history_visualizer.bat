@echo off
if not exist venv (
    echo Please run setup.bat first!
    pause
    exit /b
)

echo Starting Location History Visualizer...
call venv\Scripts\activate
streamlit run app.py
pause