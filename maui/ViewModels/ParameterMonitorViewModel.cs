using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;

namespace OBD2Tool.ViewModels
{
    public partial class ParameterMonitorViewModel : ObservableObject
    {
        private readonly IPerformanceService _perfService;
        private readonly Timer _updateTimer;
        private string _selectedGroup = "Engine";

        [ObservableProperty]
        private bool _isSelectingParameters;

        [ObservableProperty]
        private float _currentPower;

        [ObservableProperty]
        private float _currentTorque;

        [ObservableProperty]
        private float _currentBoost;

        public ObservableCollection<ParameterInfo> AllParameters { get; } = new();
        public ObservableCollection<ParameterInfo> VisibleParameters { get; } = new();
        public ObservableCollection<ChartDataPoint> SelectedParameterHistory { get; } = new();

        public ParameterMonitorViewModel(IPerformanceService perfService)
        {
            _perfService = perfService;
            InitializeParameters();
            
            // Setup high-speed updates
            _updateTimer = new Timer(UpdateParameters, null, 0, 50); // 50ms updates
        }

        [RelayCommand]
        private void SelectGroup(string group)
        {
            _selectedGroup = group;
            UpdateVisibleParameters();
        }

        [RelayCommand]
        private void ShowParameterSelection()
        {
            IsSelectingParameters = true;
        }

        [RelayCommand]
        private void CloseParameterSelection()
        {
            IsSelectingParameters = false;
            UpdateVisibleParameters();
        }

        private void InitializeParameters()
        {
            // Engine Parameters
            AddParameter("RPM", "Engine Speed", "RPM", "Engine");
            AddParameter("MAP", "Manifold Absolute Pressure", "kPa", "Engine");
            AddParameter("TPS", "Throttle Position", "%", "Engine");
            AddParameter("Load", "Engine Load", "%", "Engine");
            AddParameter("Timing", "Ignition Timing", "°", "Engine");
            AddParameter("VE", "Volumetric Efficiency", "%", "Engine");

            // Transmission Parameters
            AddParameter("Gear", "Current Gear", "", "Transmission");
            AddParameter("Speed", "Vehicle Speed", "MPH", "Transmission");
            AddParameter("TransTemp", "Transmission Temperature", "°F", "Transmission");
            AddParameter("TorqueConverter", "Torque Converter Slip", "%", "Transmission");

            // Boost Parameters
            AddParameter("BoostActual", "Actual Boost Pressure", "PSI", "Boost");
            AddParameter("BoostTarget", "Target Boost Pressure", "PSI", "Boost");
            AddParameter("BoostDuty", "Wastegate Duty Cycle", "%", "Boost");
            AddParameter("BoostTemp", "Charge Temperature", "°F", "Boost");

            // Air/Fuel Parameters
            AddParameter("AFR", "Air/Fuel Ratio", "", "AirFuel");
            AddParameter("AFRTarget", "Target Air/Fuel Ratio", "", "AirFuel");
            AddParameter("MAF", "Mass Air Flow", "g/s", "AirFuel");
            AddParameter("FuelPressure", "Fuel Pressure", "PSI", "AirFuel");
            AddParameter("FuelTemp", "Fuel Temperature", "°F", "AirFuel");

            // Temperature Parameters
            AddParameter("CoolantTemp", "Coolant Temperature", "°F", "Temperatures");
            AddParameter("OilTemp", "Oil Temperature", "°F", "Temperatures");
            AddParameter("IntakeTemp", "Intake Air Temperature", "°F", "Temperatures");
            AddParameter("ExhaustTemp", "Exhaust Gas Temperature", "°F", "Temperatures");
            AddParameter("CylinderHeadTemp", "Cylinder Head Temperature", "°F", "Temperatures");

            // Sensor Parameters
            AddParameter("O2Voltage", "O2 Sensor Voltage", "V", "Sensors");
            AddParameter("KnockVoltage", "Knock Sensor Voltage", "V", "Sensors");
            AddParameter("BatteryVoltage", "Battery Voltage", "V", "Sensors");
            AddParameter("BaroPressure", "Barometric Pressure", "kPa", "Sensors");
            AddParameter("AccelPosition", "Accelerator Position", "%", "Sensors");

            UpdateVisibleParameters();
        }

        private void AddParameter(string name, string description, string unit, string group)
        {
            AllParameters.Add(new ParameterInfo
            {
                Name = name,
                Description = description,
                Unit = unit,
                Group = group,
                IsSelected = group == "Engine" // Engine parameters selected by default
            });
        }

        private void UpdateVisibleParameters()
        {
            VisibleParameters.Clear();
            foreach (var param in AllParameters.Where(p => 
                p.IsSelected && (_selectedGroup == "Custom" || p.Group == _selectedGroup)))
            {
                VisibleParameters.Add(param);
            }
        }

        private async void UpdateParameters(object state)
        {
            try
            {
                var data = await _perfService.GetRealtimeDataAsync();
                if (data != null)
                {
                    CurrentPower = data.TorqueActual * data.EngineRPM / 5252.0f;
                    CurrentTorque = data.TorqueActual;
                    CurrentBoost = data.BoostActual;

                    // Update parameter values
                    foreach (var param in VisibleParameters)
                    {
                        param.Value = GetParameterValue(param.Name, data);
                    }

                    // Update graph for selected parameter
                    if (VisibleParameters.Any())
                    {
                        var selectedParam = VisibleParameters[0];
                        SelectedParameterHistory.Add(new ChartDataPoint
                        {
                            Timestamp = DateTime.Now,
                            Value = selectedParam.Value
                        });

                        if (SelectedParameterHistory.Count > 100)
                            SelectedParameterHistory.RemoveAt(0);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Parameter update error: {ex.Message}");
            }
        }

        private float GetParameterValue(string paramName, PerformanceData data)
        {
            // Map parameter names to actual values
            return paramName switch
            {
                "RPM" => data.EngineRPM,
                "MAP" => data.BoostActual * 6.8947f, // PSI to kPa
                "TPS" => data.ThrottlePosition,
                "AFR" => data.AirFuelRatio,
                // Add more parameter mappings
                _ => 0.0f
            };
        }
    }

    public class ParameterInfo : ObservableObject
    {
        public string Name { get; set; }
        public string Description { get; set; }
        public string Unit { get; set; }
        public string Group { get; set; }

        private bool _isSelected;
        public bool IsSelected
        {
            get => _isSelected;
            set => SetProperty(ref _isSelected, value);
        }

        private float _value;
        public float Value
        {
            get => _value;
            set => SetProperty(ref _value, value);
        }
    }
}
