import streamlit as st
import pandas as pd
import json
import folium
from folium.plugins import HeatMap
import streamlit.components.v1 as components
import plotly.express as px

st.set_page_config(page_title="Location History Visualizer", layout="wide")

st.markdown("""
    <style>
        #MainMenu {visibility: hidden;}
        footer {visibility: hidden;}
        header {visibility: hidden;}
        [data-testid="stSidebar"][aria-expanded="true"]{ min-width: 250px; max-width: 250px; }
    </style>
""", unsafe_allow_html=True)

@st.cache_data
def load_and_parse(file_content):
    data = json.loads(file_content)
    points = []
    if "semanticSegments" in data:
        for segment in data["semanticSegments"]:
            if "timelinePath" in segment:
                for p in segment["timelinePath"]:
                    try:
                        c = p["point"].replace("°", "").split(", ")
                        points.append({"lat": float(c[0]), "lon": float(c[1]), "time": p.get("time")})
                    except: continue
    df = pd.DataFrame(points)
    if not df.empty:
        df['time'] = pd.to_datetime(df['time'], utc=True)
        df['hour'] = df['time'].dt.hour
        df['day_name'] = df['time'].dt.day_name()
        cats = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']
        df['day_name'] = pd.Categorical(df['day_name'], categories=cats, ordered=True)
    return df

with st.sidebar:
    st.subheader("📁 Data")
    uploaded_file = st.file_uploader("JSON", type=['json'], label_visibility="collapsed")
    if uploaded_file:
        df_full = load_and_parse(uploaded_file.getvalue())
        if not df_full.empty:
            st.markdown("---")
            min_d, max_d = df_full['time'].min().date(), df_full['time'].max().date()
            c1, c2 = st.columns(2)
            start_d = c1.date_input("Start", min_d)
            end_d = c2.date_input("End", max_d)
            
            st.markdown("---")

            map_style = st.selectbox("Map Style:", ["Standard", "Satellite"])
            
            viz_type = st.radio("Mode:", ["Heatmap", "Points"], horizontal=True)
            if viz_type == "Heatmap":
                hm_radius = st.slider("Radius", 1, 50, 15)
                hm_blur = st.slider("Blur", 1, 50, 15)
            else:
                manual_step = st.slider("Skip", 1, 500, 1)

if uploaded_file and not df_full.empty:
    map_placeholder = st.empty()
    
    mask_date = (df_full['time'].dt.date >= start_d) & (df_full['time'].dt.date <= end_d)
    df_base = df_full.loc[mask_date]

    st.markdown("### 📊 Analysis & Filter")
    col_f1, col_f2 = st.columns([2, 1])
    with col_f1:
        hour_range = st.slider("Hours:", 0, 23, (0, 23))
    with col_f2:
        all_days = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']
        selected_days = st.multiselect("Days:", all_days, default=all_days)

    df_filtered = df_base[(df_base['hour'] >= hour_range[0]) & (df_base['hour'] <= hour_range[1]) & (df_base['day_name'].isin(selected_days))]

    if not df_filtered.empty:
        m1, m2, m3 = st.columns(3)
        m1.metric("Points", f"{len(df_filtered):,}")
        m2.metric("Period (Days)", (end_d - start_d).days)
        
        csv = df_filtered.to_csv(index=False).encode('utf-8')
        m3.download_button("📥 Export data", data=csv, file_name="export_geodata.csv", mime="text/csv")

        center = [df_filtered['lat'].mean(), df_filtered['lon'].mean()]
        
        if map_style == "Satellite":
            tiles = "https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}"
            attr = "Google Satellite"
        else:
            tiles = "OpenStreetMap"
            attr = "OpenStreetMap"

        m = folium.Map(location=center, zoom_start=13, tiles=tiles, attr=attr)
        
        if viz_type == "Heatmap":
            HeatMap(df_filtered[['lat', 'lon']].values.tolist(), radius=hm_radius, blur=hm_blur, min_opacity=0.4).add_to(m)
        else:
            df_points = df_filtered.iloc[::manual_step]
            for row in df_points.itertuples():
                folium.CircleMarker(location=[row.lat, row.lon], radius=3, color="red", fill=True, opacity=0.5, weight=1).add_to(m)
        
        with map_placeholder:
            components.html(m._repr_html_(), height=550)

        c_chart1, c_chart2 = st.columns(2)
        with c_chart1:
            fig_h = px.histogram(df_filtered, x="hour", nbins=24, range_x=[0,23], color_discrete_sequence=['#ff4b4b'])
            fig_h.update_layout(height=200, margin=dict(l=0,r=0,t=0,b=0), xaxis_title="Hour", yaxis_title=None)
            st.plotly_chart(fig_h, use_container_width=True)
        with c_chart2:
            day_counts = df_filtered['day_name'].value_counts().sort_index().reset_index()
            fig_d = px.bar(day_counts, x="day_name", y="count", color_discrete_sequence=['#3186cc'])
            fig_d.update_layout(height=200, margin=dict(l=0,r=0,t=0,b=0), xaxis_title="Day", yaxis_title=None)
            st.plotly_chart(fig_d, use_container_width=True)
    else:
        st.warning("No Data.")
