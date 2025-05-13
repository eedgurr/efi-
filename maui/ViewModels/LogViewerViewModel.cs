using System;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SkiaSharp;

namespace OBD2Tool.ViewModels
{
    public partial class LogViewerViewModel : ObservableObject
    {
        private readonly ILogExportService _logExportService;
        private readonly SimulatorService _simulatorService;
        private readonly Dictionary<DeviceType, LoggingCapabilities> _deviceCapabilities;
        private SKCanvas _graphCanvas;
        
        [ObservableProperty]
        private float _playbackSpeed = 1.0f;

        [ObservableProperty]
        private float _currentLogTime;

        [ObservableProperty]
        private bool _isPlaying;

        [ObservableProperty]
        private int _selectedLoggingRate;

        [ObservableProperty]
        private DeviceType _selectedDevice;

        [ObservableProperty]
        private string _debugInfo;

        public ObservableCollection<string> LoadedLogs { get; } = new();
        public ObservableCollection<LogParameter> VisibleParameters { get; } = new();
        public ObservableCollection<GraphOverlay> ActiveOverlays { get; } = new();

        // Device-specific logging capabilities
        private readonly Dictionary<DeviceType, LoggingCapabilities> DefaultCapabilities = new()
        {
            {
                DeviceType.SCT,
                new LoggingCapabilities
                {
                    MaxSampleRate = 1000,
                    MinSampleRate = 10,
                    SupportedRates = new[] { 10, 20, 50, 100, 200, 500, 1000 },
                    SupportsMultiBus = true,
                    MaxChannels = 32
                }
            },
            {
                DeviceType.ELM327,
                new LoggingCapabilities
                {
                    MaxSampleRate = 100,
                    MinSampleRate = 20,
                    SupportedRates = new[] { 20, 50, 100 },
                    SupportsMultiBus = false,
                    MaxChannels = 16
                }
            },
            {
                DeviceType.ESP32,
                new LoggingCapabilities
                {
                    MaxSampleRate = 500,
                    MinSampleRate = 10,
                    SupportedRates = new[] { 10, 20, 50, 100, 200, 500 },
                    SupportsMultiBus = true,
                    MaxChannels = 24
                }
            }
        };

        // Playback control commands
        [RelayCommand]
        private void Play()
        {
            IsPlaying = true;
            StartPlayback();
        }

        [RelayCommand]
        private void Pause()
        {
            IsPlaying = false;
        }

        [RelayCommand]
        private void Stop()
        {
            IsPlaying = false;
            CurrentLogTime = 0;
            UpdateDisplay();
        }

        [RelayCommand]
        private void SetPlaybackSpeed(float speed)
        {
            PlaybackSpeed = speed;
            UpdateDebugInfo($"Playback speed set to {speed}x");
        }

        [RelayCommand]
        private void JumpToTime(float timeSeconds)
        {
            CurrentLogTime = timeSeconds;
            UpdateDisplay();
            UpdateDebugInfo($"Jumped to {timeSeconds:F2} seconds");
        }

        [RelayCommand]
        private async Task SetLoggingRate(int rate)
        {
            var capabilities = DefaultCapabilities[SelectedDevice];
            if (rate >= capabilities.MinSampleRate && rate <= capabilities.MaxSampleRate)
            {
                SelectedLoggingRate = rate;
                await ConfigureDeviceLogging(rate);
                UpdateDebugInfo($"Logging rate set to {rate}Hz for {SelectedDevice}");
            }
            else
            {
                UpdateDebugInfo($"Invalid rate for {SelectedDevice}. Must be between {capabilities.MinSampleRate} and {capabilities.MaxSampleRate}Hz");
            }
        }

        private async Task ConfigureDeviceLogging(int rate)
        {
            try
            {
                var config = new DeviceConfig
                {
                    type = SelectedDevice,
                    device_config = new DeviceConfiguration
                    {
                        performance = new PerformanceConfig
                        {
                            log_interval_ms = 1000 / rate,
                            high_precision_timing = rate > 100
                        }
                    }
                };

                await _logExportService.UpdateDeviceConfig(config);
                UpdateDebugInfo($"Device configured for {rate}Hz logging");
            }
            catch (Exception ex)
            {
                UpdateDebugInfo($"Error configuring device: {ex.Message}");
            }
        }

        private void UpdateDisplay()
        {
            if (_graphCanvas == null) return;

            // Clear canvas
            _graphCanvas.Clear(SKColors.Black);

            foreach (var overlay in ActiveOverlays)
            {
                DrawOverlay(overlay);
            }

            // Draw time marker
            DrawTimeMarker();

            // Update parameter values
            UpdateParameterValues();
        }

        private void DrawOverlay(GraphOverlay overlay)
        {
            using var paint = new SKPaint
            {
                Color = overlay.Color,
                Style = SKPaintStyle.Stroke,
                StrokeWidth = 2,
                IsAntialias = true
            };

            // Apply transparency
            paint.Color = paint.Color.WithAlpha((byte)(255 * overlay.Transparency));

            var path = new SKPath();
            bool first = true;

            foreach (var point in overlay.DataPoints)
            {
                if (point.Time > CurrentLogTime - 30 && point.Time <= CurrentLogTime)
                {
                    float x = MapTimeToX(point.Time);
                    float y = MapValueToY(point.Value, overlay.MinValue, overlay.MaxValue);

                    if (first)
                    {
                        path.MoveTo(x, y);
                        first = false;
                    }
                    else
                    {
                        path.LineTo(x, y);
                    }
                }
            }

            _graphCanvas.DrawPath(path, paint);
        }

        private float MapTimeToX(float time)
        {
            // Map time to X coordinate
            return (time - (CurrentLogTime - 30)) / 30 * _graphCanvas.DeviceClipBounds.Width;
        }

        private float MapValueToY(float value, float min, float max)
        {
            // Map value to Y coordinate
            return (1 - (value - min) / (max - min)) * _graphCanvas.DeviceClipBounds.Height;
        }

        private void DrawTimeMarker()
        {
            using var paint = new SKPaint
            {
                Color = SKColors.White,
                Style = SKPaintStyle.Stroke,
                StrokeWidth = 1
            };

            float x = _graphCanvas.DeviceClipBounds.Width;
            _graphCanvas.DrawLine(x, 0, x, _graphCanvas.DeviceClipBounds.Height, paint);
        }

        private void UpdateParameterValues()
        {
            foreach (var param in VisibleParameters)
            {
                param.CurrentValue = GetParameterValueAtTime(param.Name, CurrentLogTime);
            }
        }

        private float GetParameterValueAtTime(string paramName, float time)
        {
            // Find the closest data point for the parameter at the given time
            var overlay = ActiveOverlays.FirstOrDefault(o => o.ParameterName == paramName);
            if (overlay == null) return 0;

            var point = overlay.DataPoints
                .OrderBy(p => Math.Abs(p.Time - time))
                .FirstOrDefault();

            return point?.Value ?? 0;
        }

        private void UpdateDebugInfo(string message)
        {
            DebugInfo = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
        }
    }

    public class LoggingCapabilities
    {
        public int MaxSampleRate { get; set; }
        public int MinSampleRate { get; set; }
        public int[] SupportedRates { get; set; }
        public bool SupportsMultiBus { get; set; }
        public int MaxChannels { get; set; }
    }

    public class GraphOverlay
    {
        public string ParameterName { get; set; }
        public SKColor Color { get; set; }
        public float Transparency { get; set; }
        public float MinValue { get; set; }
        public float MaxValue { get; set; }
        public List<DataPoint> DataPoints { get; set; } = new();
    }

    public class DataPoint
    {
        public float Time { get; set; }
        public float Value { get; set; }
    }

    public class LogParameter
    {
        public string Name { get; set; }
        public string Unit { get; set; }
        public float CurrentValue { get; set; }
        public bool IsVisible { get; set; }
    }
}
