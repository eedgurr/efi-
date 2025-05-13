using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using OBD2Tool.Services;
using System.Text.Json;
using System.Collections.ObjectModel;

namespace OBD2Tool.ViewModels
{
    public partial class CashoutDashboardViewModel : ObservableObject
    {
        private readonly IPerformanceService _perfService;
        private readonly ILogExportService _logService;
        private readonly SimulatorService _simulatorService;
        private VehicleProfile _currentProfile;
        private JsonElement _brandConfig;

        [ObservableProperty]
        private string _selectedVehicleType;

        [ObservableProperty]
        private float _currentRPM;

        [ObservableProperty]
        private float _maxRPM;

        [ObservableProperty]
        private float _redlineStart;

        [ObservableProperty]
        private float _currentBoost;

        [ObservableProperty]
        private float _maxBoost;

        [ObservableProperty]
        private float _safeBoostLimit;

        [ObservableProperty]
        private float _currentAFR;

        [ObservableProperty]
        private float _oilTemp;

        [ObservableProperty]
        private float _coolantTemp;

        [ObservableProperty]
        private float _timingAdvance;

        [ObservableProperty]
        private string _specialParamName;

        [ObservableProperty]
        private string _specialParamValue;

        [ObservableProperty]
        private string _statusMessage;

        [ObservableProperty]
        private int _loggingRate;

        [ObservableProperty]
        private bool _isConnected;

        [ObservableProperty]
        private bool _isLogging;

        [ObservableProperty]
        private string _connectButtonText = "Connect";

        [ObservableProperty]
        private string _loggingButtonText = "Start Log";

        public ObservableCollection<string> VehicleTypes { get; } = new();

        public CashoutDashboardViewModel(
            IPerformanceService perfService,
            ILogExportService logService,
            SimulatorService simulatorService)
        {
            _perfService = perfService;
            _logService = logService;
            _simulatorService = simulatorService;

            LoadBrandConfig();
            InitializeVehicleTypes();
        }

        private void LoadBrandConfig()
        {
            var configPath = Path.Combine(
                AppContext.BaseDirectory,
                "Config",
                "brand_config.json"
            );

            var jsonString = File.ReadAllText(configPath);
            _brandConfig = JsonSerializer.Deserialize<JsonElement>(jsonString);

            // Load vehicle profiles
            var profiles = _brandConfig.GetProperty("vehicleProfiles");
            foreach (var profile in profiles.EnumerateObject())
            {
                VehicleTypes.Add(profile.Name);
            }
        }

        private void InitializeVehicleTypes()
        {
            SelectedVehicleType = VehicleTypes.FirstOrDefault();
            UpdateVehicleProfile();
        }

        partial void OnSelectedVehicleTypeChanged(string value)
        {
            UpdateVehicleProfile();
        }

        private void UpdateVehicleProfile()
        {
            var profile = _brandConfig
                .GetProperty("vehicleProfiles")
                .GetProperty(SelectedVehicleType);

            var safetyLimits = profile.GetProperty("safetyLimits");

            _currentProfile = new VehicleProfile
            {
                Name = profile.GetProperty("name").GetString(),
                MaxRPM = safetyLimits.GetProperty("maxRPM").GetInt32(),
                MaxBoost = safetyLimits.GetProperty("maxBoost").GetInt32(),
                MaxOilTemp = safetyLimits.TryGetProperty("maxOilTemp", out var oilTemp) ? oilTemp.GetInt32() : 280,
                DefaultLoggingRate = profile.GetProperty("loggingRates").GetProperty("default").GetInt32()
            };

            // Update UI limits
            MaxRPM = _currentProfile.MaxRPM;
            RedlineStart = MaxRPM - 500;
            MaxBoost = _currentProfile.MaxBoost;
            SafeBoostLimit = MaxBoost - 5;
            LoggingRate = _currentProfile.DefaultLoggingRate;

            // Update special parameter based on vehicle type
            UpdateSpecialParameter();
        }

        private void UpdateSpecialParameter()
        {
            switch (SelectedVehicleType.ToLower())
            {
                case "mustang":
                    SpecialParamName = "Trans Temp";
                    break;
                case "bmw":
                    SpecialParamName = "VANOS";
                    break;
                case "honda":
                    SpecialParamName = "VTEC";
                    break;
                case "diesel":
                    SpecialParamName = "EGT";
                    break;
            }
        }

        [RelayCommand]
        private async Task ConnectAsync()
        {
            try
            {
                IsConnected = await _perfService.ConnectAsync();
                if (IsConnected)
                {
                    StatusMessage = "Connected";
                    ConnectButtonText = "Disconnect";
                    StartDataMonitoring();
                }
                else
                {
                    StatusMessage = "Connection failed";
                    ConnectButtonText = "Connect";
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error: {ex.Message}";
                IsConnected = false;
                ConnectButtonText = "Connect";
            }
        }

        [RelayCommand]
        private async Task StartLogging()
        {
            try
            {
                if (!IsLogging)
                {
                    // Configure logging rate based on vehicle profile
                    await _perfService.SetLoggingRate(_currentProfile.DefaultLoggingRate);
                    
                    // Start logging
                    await _perfService.StartLoggingAsync();
                    IsLogging = true;
                    LoggingButtonText = "Stop Log";
                    StatusMessage = "Logging started";
                }
                else
                {
                    await _perfService.StopLoggingAsync();
                    IsLogging = false;
                    LoggingButtonText = "Start Log";
                    StatusMessage = "Logging stopped";

                    // Auto-export log
                    var logPath = await _logService.ExportSessionToCSV(
                        $"{SelectedVehicleType}_{DateTime.Now:yyyyMMdd_HHmmss}",
                        await _perfService.GetSessionDataAsync()
                    );
                    StatusMessage = $"Log saved: {logPath}";
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Logging error: {ex.Message}";
            }
        }

        private void StartDataMonitoring()
        {
            // Start real-time updates
            Device.StartTimer(TimeSpan.FromMilliseconds(50), () =>
            {
                UpdateRealTimeData();
                return IsConnected;
            });
        }

        private async void UpdateRealTimeData()
        {
            try
            {
                var data = await _perfService.GetRealtimeDataAsync();
                
                CurrentRPM = data.EngineRPM;
                CurrentBoost = data.BoostActual;
                CurrentAFR = data.AirFuelRatio;
                OilTemp = data.SensorData.OilTemp;
                CoolantTemp = data.SensorData.CoolantTemp;
                TimingAdvance = data.TimingAdvance;

                // Update special parameter based on vehicle type
                UpdateSpecialValue(data);

                // Check safety limits
                CheckSafetyLimits(data);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Data update error: {ex.Message}");
            }
        }

        private void CheckSafetyLimits(PerformanceData data)
        {
            var warnings = new List<string>();

            if (data.EngineRPM >= _currentProfile.MaxRPM)
                warnings.Add("RPM LIMIT");
            if (data.BoostActual >= _currentProfile.MaxBoost)
                warnings.Add("BOOST LIMIT");
            if (data.SensorData.OilTemp >= _currentProfile.MaxOilTemp)
                warnings.Add("OIL TEMP");

            if (warnings.Count > 0)
            {
                StatusMessage = $"WARNING: {string.Join(", ", warnings)}";
            }
        }

        private void UpdateSpecialValue(PerformanceData data)
        {
            switch (SelectedVehicleType.ToLower())
            {
                case "mustang":
                    SpecialParamValue = $"{data.SensorData.TransTemp:F0}°F";
                    break;
                case "bmw":
                    SpecialParamValue = $"{data.VanosPosition:F0}°";
                    break;
                case "honda":
                    SpecialParamValue = data.VtecEngaged ? "Engaged" : "Off";
                    break;
                case "diesel":
                    SpecialParamValue = $"{data.SensorData.Egt[0]:F0}°F";
                    break;
            }
        }

        [RelayCommand]
        private Task ViewLogs()
        {
            return Shell.Current.GoToAsync("//LogViewer");
        }

        [RelayCommand]
        private Task ShowSettings()
        {
            return Shell.Current.GoToAsync("//Settings");
        }
    }

    public class VehicleProfile
    {
        public string Name { get; set; }
        public int MaxRPM { get; set; }
        public int MaxBoost { get; set; }
        public int MaxOilTemp { get; set; }
        public int DefaultLoggingRate { get; set; }
    }
}
