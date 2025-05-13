#!/usr/bin/env python3
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import folium
from datetime import datetime
import cv2
import json
from pathlib import Path

class TelemetryAnalyzer:
    def __init__(self, telemetry_file, video_file=None):
        self.telemetry_data = pd.read_csv(telemetry_file)
        self.video = cv2.VideoCapture(video_file) if video_file else None
        self.sync_offset = 0  # Video sync offset in milliseconds
        
    def generate_track_map(self, output_file='track_map.html'):
        """Generate an interactive track map with telemetry data"""
        # Calculate center point
        center_lat = self.telemetry_data['lat'].mean()
        center_lon = self.telemetry_data['lon'].mean()
        
        # Create map
        m = folium.Map(location=[center_lat, center_lon], zoom_start=15)
        
        # Add track path with speed colors
        speeds = self.telemetry_data['speed']
        normalized_speeds = (speeds - speeds.min()) / (speeds.max() - speeds.min())
        
        points = list(zip(self.telemetry_data['lat'], 
                         self.telemetry_data['lon']))
                         
        for i in range(len(points) - 1):
            color = f'#{int(255 * (1-normalized_speeds[i])):02x}' + \
                    f'00{int(255 * normalized_speeds[i]):02x}'
            folium.PolyLine(
                locations=[points[i], points[i+1]],
                color=color,
                weight=2,
                opacity=0.8
            ).add_to(m)
        
        m.save(output_file)
        
    def plot_performance_data(self, output_file='performance.png'):
        """Generate performance plots"""
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        
        # Speed vs Time
        ax1.plot(self.telemetry_data['timestamp'], 
                self.telemetry_data['speed'])
        ax1.set_title('Speed vs Time')
        ax1.set_xlabel('Time (s)')
        ax1.set_ylabel('Speed (mph)')
        
        # RPM vs Time
        ax2.plot(self.telemetry_data['timestamp'], 
                self.telemetry_data['rpm'])
        ax2.set_title('RPM vs Time')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('RPM')
        
        # G-Force
        ax3.plot(self.telemetry_data['accel_x'], 
                self.telemetry_data['accel_y'], 'r.')
        ax3.set_title('G-Force Plot')
        ax3.set_xlabel('Lateral G')
        ax3.set_ylabel('Longitudinal G')
        ax3.grid(True)
        ax3.axhline(y=0, color='k', linestyle='-', alpha=0.3)
        ax3.axvline(x=0, color='k', linestyle='-', alpha=0.3)
        
        # Boost vs RPM
        ax4.scatter(self.telemetry_data['rpm'], 
                   self.telemetry_data['boost'],
                   c=self.telemetry_data['speed'],
                   cmap='viridis',
                   alpha=0.5)
        ax4.set_title('Boost vs RPM')
        ax4.set_xlabel('RPM')
        ax4.set_ylabel('Boost (psi)')
        
        plt.tight_layout()
        plt.savefig(output_file)
        
    def create_video_overlay(self, output_file='output_video.mp4'):
        """Create video with telemetry overlay"""
        if not self.video:
            return
            
        # Get video properties
        width = int(self.video.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(self.video.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = int(self.video.get(cv2.CAP_PROP_FPS))
        
        # Create video writer
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter(output_file, fourcc, fps, (width, height))
        
        frame_count = 0
        while self.video.isOpened():
            ret, frame = self.video.read()
            if not ret:
                break
                
            # Get telemetry data for current frame
            timestamp = (frame_count / fps * 1000) + self.sync_offset
            current_data = self.get_telemetry_at_time(timestamp)
            
            if current_data is not None:
                # Add telemetry overlay
                self._draw_telemetry_overlay(frame, current_data)
            
            out.write(frame)
            frame_count += 1
            
        self.video.release()
        out.release()
        
    def _draw_telemetry_overlay(self, frame, data):
        """Draw telemetry overlay on video frame"""
        h, w = frame.shape[:2]
        
        # Add semi-transparent background
        overlay = frame.copy()
        cv2.rectangle(overlay, (10, h-150), (300, h-10), (0, 0, 0), -1)
        frame = cv2.addWeighted(overlay, 0.3, frame, 0.7, 0)
        
        # Add telemetry text
        font = cv2.FONT_HERSHEY_SIMPLEX
        cv2.putText(frame, f"Speed: {data['speed']:.0f} mph", 
                   (20, h-120), font, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"RPM: {data['rpm']:.0f}", 
                   (20, h-90), font, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"Boost: {data['boost']:.1f} psi", 
                   (20, h-60), font, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"G-Force: {data['g_force']:.2f}", 
                   (20, h-30), font, 0.7, (255, 255, 255), 2)
        
        # Add lap time if available
        if 'lap_time' in data:
            cv2.putText(frame, f"Lap: {data['lap_time']:.3f}", 
                       (w-200, 30), font, 0.7, (255, 255, 255), 2)
        
    def get_telemetry_at_time(self, timestamp_ms):
        """Get telemetry data at specific timestamp"""
        idx = (self.telemetry_data['timestamp'] - timestamp_ms).abs().idxmin()
        return self.telemetry_data.iloc[idx]
        
    def export_lap_report(self, output_file='lap_report.pdf'):
        """Generate detailed lap report with analysis"""
        # Implement lap report generation
        pass

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='Telemetry Analysis Tool')
    parser.add_argument('telemetry_file', help='Path to telemetry CSV file')
    parser.add_argument('--video', help='Path to video file')
    parser.add_argument('--output-dir', default='analysis_output',
                      help='Output directory for analysis files')
    
    args = parser.parse_args()
    
    # Create output directory
    Path(args.output_dir).mkdir(parents=True, exist_ok=True)
    
    # Initialize analyzer
    analyzer = TelemetryAnalyzer(args.telemetry_file, args.video)
    
    # Generate analysis
    analyzer.generate_track_map(f'{args.output_dir}/track_map.html')
    analyzer.plot_performance_data(f'{args.output_dir}/performance.png')
    
    if args.video:
        analyzer.create_video_overlay(f'{args.output_dir}/overlay_video.mp4')
