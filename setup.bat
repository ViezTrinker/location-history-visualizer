@echo off
echo ======================================================
echo Installing Location History Visualizer...
echo ======================================================

:: Check if python is installed
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python is not installed. Please install Python 3.8+ first.
    pause
    exit /b
)

:: Create virtual environment
echo Creating virtual environment (venv)...
python -m venv venv

:: Install libraries
echo Installing dependencies...
call venv\Scripts\activate
python -m pip install --upgrade pip
pip install streamlit pandas folium plotly geopy

echo ======================================================================
echo Setup complete! Now you can use start_location_history_visualizer.bat
echo ======================================================================
pause