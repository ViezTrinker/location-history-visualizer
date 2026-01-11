# 📍 Location History Visualizer

A professional, high-performance Streamlit dashboard to visualize and analyze your Google Location History (JSON). This tool allows you to filter through thousands of data points by date, time, and weekday with real-time heatmap and satellite view integration.

## ✨ Features
* **Interactive Maps:** Switch between OpenStreetMap and Google Satellite views.
* **Heatmap & Point Mode:** Toggle between density visualization and individual data points.
* **S-Tier Filtering:** Deep-dive filters for specific hours of the day and weekdays.
* **Analytics Dashboard:** Interactive Plotly charts showing activity distribution.
* **Distance Tracker:** Automatically estimates the total distance traveled (km).
* **Data Export:** Export your filtered results directly as a CSV file.

## 🚀 Quick Start
1.  **Install dependencies:**
    ```bash
    pip install streamlit pandas folium plotly geopy
    ```
2.  **Run the app:**
    ```bash
    python -m streamlit run app.py
    ```
    *Or simply double-click the `start_history_location_visualizer.bat` (Windows).*

3.  **Upload your data:** Use the exported JSON File with Location data

## 🛠️ Requirements
* Python 3.8+
* Streamlit, Folium, Plotly, Pandas, Geopy