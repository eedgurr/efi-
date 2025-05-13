using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using OBD2Tool.Services;
using System.Collections.ObjectModel;

namespace OBD2Tool.ViewModels
{
    public partial class RaceDashboardViewModel : ObservableObject
    {
        private readonly IPerformanceService _perfService;
        private readonly HighPerformanceLogger _logger;
        private bool _isNightMode;
        private bool _isLogging;
        private readonly Timer _updateTimer;

        [ObservableProperty]
        private float _currentRPM;

        [ObservableProperty]
        private float _maxRPM = 8000;

        [ObservableProperty]
        private float _redlineStart = 7000;

        [ObservableProperty]
        private float _currentBoost;

        [ObservableProperty]
        private float _maxBoost = 30;

        [ObservableProperty]
        private float _currentAFR;

        [ObservableProperty]
        private float _targetAFR = 14.7f;

        [ObservableProperty]
        private float _currentSpeed;

        [ObservableProperty]
        private int _currentGear;

        [ObservableProperty]
        private float _currentPower;

        [ObservableProperty]
        private string _zeroToSixty = "--";

        [ObservableProperty]
        private string _eighthMileTime = "--";

        [ObservableProperty]
        private string _quarterMileTime = "--";

        [ObservableProperty]
        private string _sixtyTo130Time = "--";

        [ObservableProperty]
        private string _hundredTo150Time = "--";

        [ObservableProperty]
        private string _connectionStatus = "Disconnected";

        [ObservableProperty]
        private Color _statusColor = Colors.Red;

        [ObservableProperty]
        private Color _dashboardBackground = Colors.Black;

        [ObservableProperty]
        private float _displayBrightness = 1.0f;

        [ObservableProperty]
        private Color _loggingButtonColor = Colors.Green;

        public ObservableCollection<ChartDataPoint> PowerHistory { get; } = new();

        public RaceDashboardViewModel(IPerformanceService perfService, HighPerformanceLogger logger)
        {
            _perfService = perfService;
            _logger = logger;

            // Setup high-speed updates
            _updateTimer = new Timer(UpdateDashboard, null, 0, 10); // 10ms updates
            
            // Subscribe to real-time data updates
            _logger.RealTimeDataUpdated += OnRealTimeDataUpdated;
        }

        [RelayCommand]
        private async Task ToggleLogging()
        {
            if (!_isLogging)
            {
                // Start new logging session
                _isLogging = true;
                LoggingButtonColor = Colors.Red;
                await _logger.StartHighPrecisionLogging();
            }
            else
            {
                // Stop logging
                _isLogging = false;
                LoggingButtonColor = Colors.Green;
                _logger.StopLogging();

                // Save the log
                var logPath = await _logger.SaveLogAsync($"DragRun_{DateTime.Now:yyyyMMdd_HHmmss}");
            }
        }

        [RelayCommand]
        private void ToggleNightMode()
        {
            _isNightMode = !_isNightMode;
            UpdateNightMode();
        }

        private void UpdateNightMode()
        {
            if (_isNightMode)
            {
                DashboardBackground = Colors.Black;
                DisplayBrightness = 0.7f; // Reduced brightness
            }
            else
            {
                DashboardBackground = Colors.White;
                DisplayBrightness = 1.0f;
            }
        }

        private async void UpdateDashboard(object state)
        {
            try
            {
                var data = await _perfService.GetRealtimeDataAsync();
                if (data != null)
                {
                    CurrentRPM = data.EngineRPM;
                    CurrentBoost = data.BoostActual;
                    CurrentAFR = data.AirFuelRatio;
                    CurrentSpeed = data.VehicleSpeed;
                    CurrentGear = CalculateGear(data);
                    CurrentPower = CalculatePower(data);

                    // Update performance history
                    PowerHistory.Add(new ChartDataPoint 
                    { 
                        Timestamp = DateTime.Now,
                        Value = CurrentPower 
                    });

                    if (PowerHistory.Count > 100) // Keep last 100 points
                        PowerHistory.RemoveAt(0);

                    // Update timing if we're logging
                    if (_isLogging)
                    {
                        UpdatePerformanceMetrics(data);
                    }

                    // Update connection status
                    ConnectionStatus = "Connected";
                    StatusColor = Colors.Green;
                }
            }
            catch (Exception ex)
            {
                ConnectionStatus = $"Error: {ex.Message}";
                StatusColor = Colors.Red;
            }
        }

        private void OnRealTimeDataUpdated(object sender, PerformanceData data)
        {
            // Update real-time overlay data
            MainThread.BeginInvokeOnMainThread(() =>
            {
                // Update any additional real-time metrics
                UpdatePerformanceMetrics(data);
            });
        }

        private async void UpdatePerformanceMetrics(PerformanceData data)
        {
            // Calculate 0-60 time if we don't have it yet
            if (ZeroToSixty == "--" && data.VehicleSpeed >= 60)
            {
                ZeroToSixty = CalculateZeroToSixty(data);
            }

            // Calculate 1/8 mile time
            if (EighthMileTime == "--")
            {
                var eighthMileData = CalculateEighthMileTime(data);
                if (eighthMileData.hasReached)
                {
                    EighthMileTime = eighthMileData.time;
                }
            }

            // Calculate 1/4 mile time
            if (QuarterMileTime == "--")
            {
                var quarterMileData = CalculateQuarterMileTime(data);
                if (quarterMileData.hasReached)
                {
                    QuarterMileTime = quarterMileData.time;
                }
            }

            // Update speed range times from session data if available
            var sessionData = await _perfService.GetSessionDataAsync();
            if (sessionData != null)
            {
                if (sessionData.sixty_to_130.completed && SixtyTo130Time == "--")
                {
                    SixtyTo130Time = $"{sessionData.sixty_to_130.elapsed_time:F2}s";
                }

                if (sessionData.hundred_to_150.completed && HundredTo150Time == "--")
                {
                    HundredTo150Time = $"{sessionData.hundred_to_150.elapsed_time:F2}s";
                }
            }
        }

        private int CalculateGear(PerformanceData data)
        {
            // Implement gear calculation based on RPM and speed
            return 1; // Placeholder
        }

        private float CalculatePower(PerformanceData data)
        {
            // Calculate power from torque and RPM
            return (data.TorqueActual * data.EngineRPM) / 5252.0f;
        }

        private string CalculateZeroToSixty(PerformanceData data)
        {
            // Implement 0-60 calculation
            return "4.5s"; // Placeholder
        }

        private (bool hasReached, string time) CalculateEighthMileTime(PerformanceData data)
        {
            // Implement 1/8 mile calculation
            return (false, "--"); // Placeholder
        }

        private (bool hasReached, string time) CalculateQuarterMileTime(PerformanceData data)
        {
            // Implement 1/4 mile calculation
            return (false, "--"); // Placeholder
        }
    }

    public class ChartDataPoint
    {
        public DateTime Timestamp { get; set; }
        public float Value { get; set; }
    }
}
