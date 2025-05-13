using System.Collections.Concurrent;
using OBD2Tool.Models;

namespace OBD2Tool.Services
{
    public class SimulatorService
    {
        private readonly Random _random = new Random();
        private readonly ConcurrentQueue<PerformanceData> _simulationData;
        private readonly Timer _simulationTimer;
        private SimulationConfig _config;
        private bool _isRunning;
        private DateTime _startTime;
        private float _baseRpm = 800;
        private float _targetRpm = 800;
        private float _currentBoost = 0;
        private float _targetBoost = 0;

        public event EventHandler<PerformanceData> SimulatedDataAvailable;

        public SimulatorService()
        {
            _simulationData = new ConcurrentQueue<PerformanceData>();
            _simulationTimer = new Timer(SimulationCallback, null, Timeout.Infinite, Timeout.Infinite);
        }

        public void Configure(SimulationConfig config)
        {
            _config = config;
        }

        public async Task StartSimulation()
        {
            if (_isRunning) return;

            _isRunning = true;
            _startTime = DateTime.Now;

            if (_config.replay_config.replay_mode)
            {
                await LoadReplayData();
                StartReplay();
            }
            else
            {
                StartRealTimeSimulation();
            }
        }

        public void StopSimulation()
        {
            _isRunning = false;
            _simulationTimer.Change(Timeout.Infinite, Timeout.Infinite);
            _simulationData.Clear();
        }

        private async Task LoadReplayData()
        {
            _simulationData.Clear();
            
            if (File.Exists(_config.replay_config.replay_file))
            {
                using var reader = new StreamReader(_config.replay_config.replay_file);
                using var csv = new CsvHelper.CsvReader(reader, System.Globalization.CultureInfo.InvariantCulture);
                
                var records = csv.GetRecords<PerformanceData>();
                foreach (var record in records)
                {
                    _simulationData.Enqueue(record);
                }
            }
        }

        private void StartReplay()
        {
            int interval = (int)(1000 / (_config.replay_config.replay_speed * 60)); // 60Hz base rate
            _simulationTimer.Change(0, interval);
        }

        private void StartRealTimeSimulation()
        {
            int interval = (int)(1000 / (_config.simulation_multiplier * 60)); // 60Hz base rate
            _simulationTimer.Change(0, interval);
        }

        private void SimulationCallback(object state)
        {
            if (!_isRunning) return;

            if (_config.replay_config.replay_mode)
            {
                if (_simulationData.TryDequeue(out var replayData))
                {
                    SimulatedDataAvailable?.Invoke(this, AddNoise(replayData));
                }
                else
                {
                    StopSimulation();
                }
            }
            else
            {
                var simulatedData = GenerateSimulatedData();
                SimulatedDataAvailable?.Invoke(this, simulatedData);
            }
        }

        private PerformanceData GenerateSimulatedData()
        {
            UpdateSimulationState();

            var data = new PerformanceData
            {
                TimestampUs = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() * 1000,
                EngineRPM = _baseRpm + GetRandomVariance(_config.simulation_params.rpm_variance),
                BoostActual = _currentBoost + GetRandomVariance(_config.simulation_params.boost_variance),
                AirFuelRatio = 14.7f + GetRandomVariance(_config.simulation_params.afr_variance),
                ThrottlePosition = CalculateThrottlePosition(),
                Load = CalculateEngineLoad(),
                
                // Additional simulated sensor data
                SensorData = new SensorData
                {
                    OilPressure = SimulateOilPressure(),
                    CoolantTemp = SimulateCoolantTemp(),
                    OilTemp = SimulateOilTemp(),
                    TransTemp = SimulateTransTemp(),
                    FuelPressure = SimulateFuelPressure(),
                    TirePressure = SimulateTirePressures(),
                    TireTemp = SimulateTireTemps(),
                    Egt = SimulateEgtValues(),
                    KnockLevel = SimulateKnockLevels()
                },

                // Safety status
                SafetyStatus = new SafetyStatus
                {
                    InSafeRange = true,
                    WarningFlags = 0,
                    WarningMessage = "",
                    SafetyMargin = 1.0f
                }
            };

            // Calculate derived values
            data.VolumetricEfficiency = CalculateVE(data);
            data.TorqueActual = CalculateTorque(data);
            data.CurrentPower = CalculatePower(data);

            return data;
        }

        private void UpdateSimulationState()
        {
            var elapsed = (DateTime.Now - _startTime).TotalSeconds * _config.simulation_multiplier;

            // Simulate acceleration run
            if (elapsed < 15) // 0-15 seconds
            {
                _targetRpm = 7500;
                _targetBoost = 25;
            }
            else if (elapsed < 16) // Shift
            {
                _targetRpm = 5000;
                _targetBoost = 15;
            }
            else if (elapsed < 20) // Accelerate again
            {
                _targetRpm = 7500;
                _targetBoost = 25;
            }
            else // Cool down
            {
                _targetRpm = 800;
                _targetBoost = 0;
            }

            // Smooth transitions
            _baseRpm += (_targetRpm - _baseRpm) * 0.1f;
            _currentBoost += (_targetBoost - _currentBoost) * 0.1f;
        }

        private float GetRandomVariance(float variance)
        {
            return _config.simulation_params.add_sensor_noise ?
                (float)(_random.NextDouble() * 2 - 1) * variance * _config.simulation_params.noise_amplitude :
                0;
        }

        private PerformanceData AddNoise(PerformanceData data)
        {
            if (!_config.simulation_params.add_sensor_noise)
                return data;

            data.EngineRPM += GetRandomVariance(_config.simulation_params.rpm_variance);
            data.BoostActual += GetRandomVariance(_config.simulation_params.boost_variance);
            data.AirFuelRatio += GetRandomVariance(_config.simulation_params.afr_variance);

            return data;
        }

        private float CalculateThrottlePosition()
        {
            return (_baseRpm - 800) / (7500 - 800) * 100;
        }

        private float CalculateEngineLoad()
        {
            return (_baseRpm * _currentBoost) / (7500 * 25) * 100;
        }

        private float SimulateOilPressure()
        {
            return 60 + (_baseRpm / 7500 * 30) + GetRandomVariance(5);
        }

        private float SimulateCoolantTemp()
        {
            return 180 + (_baseRpm / 7500 * 20) + GetRandomVariance(2);
        }

        private float SimulateOilTemp()
        {
            return 190 + (_baseRpm / 7500 * 30) + GetRandomVariance(3);
        }

        private float SimulateTransTemp()
        {
            return 170 + (_baseRpm / 7500 * 25) + GetRandomVariance(2);
        }

        private float SimulateFuelPressure()
        {
            return 40 + (_currentBoost * 1.5f) + GetRandomVariance(2);
        }

        private float[] SimulateTirePressures()
        {
            return new float[4]
            {
                32 + GetRandomVariance(0.5f),
                32 + GetRandomVariance(0.5f),
                30 + GetRandomVariance(0.5f),
                30 + GetRandomVariance(0.5f)
            };
        }

        private float[] SimulateTireTemps()
        {
            var baseTemp = 70 + (_baseRpm / 7500 * 30);
            return new float[4]
            {
                baseTemp + GetRandomVariance(5),
                baseTemp + GetRandomVariance(5),
                baseTemp + GetRandomVariance(5),
                baseTemp + GetRandomVariance(5)
            };
        }

        private float[] SimulateEgtValues()
        {
            var baseEgt = 1200 + (_currentBoost * 10);
            return new float[8]
            {
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50),
                baseEgt + GetRandomVariance(50)
            };
        }

        private float[] SimulateKnockLevels()
        {
            var baseKnock = (_baseRpm / 7500 * _currentBoost / 25) * 2;
            return new float[8]
            {
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f),
                baseKnock + GetRandomVariance(0.5f)
            };
        }

        private float CalculateVE(PerformanceData data)
        {
            // Simplified VE calculation
            return (data.EngineRPM / 7500) * (data.BoostActual / 25) * 100;
        }

        private float CalculateTorque(PerformanceData data)
        {
            // Simplified torque calculation
            return (data.EngineRPM / 7500) * (data.BoostActual / 25) * 500;
        }

        private float CalculatePower(PerformanceData data)
        {
            return (data.TorqueActual * data.EngineRPM) / 5252.0f;
        }
    }
}
