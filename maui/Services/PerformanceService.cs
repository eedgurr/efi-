using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using OBD2Tool.Models;

namespace OBD2Tool.Services
{
    public interface IPerformanceService
    {
        Task<bool> StartDragSessionAsync();
        Task<bool> StopDragSessionAsync();
        Task<PerformanceData> GetRealtimeDataAsync();
        Task<DragSessionData> GetSessionDataAsync();
        Task SetDisplayBrightnessAsync(byte level);
        Task SetCanProtocolAsync(byte canBus, uint baudRate);
        Task<bool> ConfigureLoggingAsync(uint intervalMs);
    }

    public class PerformanceService : IPerformanceService
    {
        private readonly IOBDService _obdService;
        private bool _isLogging;
        private readonly object _lockObject = new object();
        private readonly List<PerformanceData> _sessionData = new List<PerformanceData>();
        private SpeedRangeTime _60to130 = new SpeedRangeTime { start_speed = 60, end_speed = 130 };
        private SpeedRangeTime _100to150 = new SpeedRangeTime { start_speed = 100, end_speed = 150 };

        public PerformanceService(IOBDService obdService)
        {
            _obdService = obdService;
        }

        public async Task<bool> StartDragSessionAsync()
        {
            lock (_lockObject)
            {
                if (_isLogging)
                    return false;

                _sessionData.Clear();
                _isLogging = true;
            }

            // Start background logging task
            _ = Task.Run(async () =>
            {
                while (_isLogging)
                {
                    var data = await GetRealtimeDataAsync();
                    lock (_lockObject)
                    {
                        _sessionData.Add(data);
                    }
                    await Task.Delay(10); // 10ms sampling rate for high precision
                }
            });

            return true;
        }

        public Task<bool> StopDragSessionAsync()
        {
            lock (_lockObject)
            {
                _isLogging = false;
            }
            return Task.FromResult(true);
        }

        public async Task<PerformanceData> GetRealtimeDataAsync()
        {
            // Get raw sensor data
            var mafData = await _obdService.RequestPIDAsync("0110"); // MAF sensor
            var rpmData = await _obdService.RequestPIDAsync("010C"); // Engine RPM
            var speedData = await _obdService.RequestPIDAsync("010D"); // Vehicle speed
            var mapData = await _obdService.RequestPIDAsync("010B"); // Intake manifold pressure
            var iatData = await _obdService.RequestPIDAsync("010F"); // Intake air temp
            var tpsData = await _obdService.RequestPIDAsync("0111"); // Throttle position
            var sparkData = await _obdService.RequestPIDAsync("010E"); // Timing advance
            var afrData = await _obdService.RequestPIDAsync("0144"); // Air-fuel ratio

            // Calculate performance metrics
            var ve = CalculateVE(
                mafData.Value,
                rpmData.Value,
                mapData.Value,
                iatData.Value
            );

            var torque = CalculateTorque(
                mafData.Value,
                rpmData.Value,
                sparkData.Value
            );

            var data = new PerformanceData
            {
                Timestamp = DateTime.UtcNow,
                VolumetricEfficiency = ve,
                MafScaled = ScaleMAF(mafData.Value, iatData.Value, 101.325f), // Assuming sea level
                TorqueActual = torque,
                EngineRPM = rpmData.Value,
                VehicleSpeed = speedData.Value,
                ThrottlePosition = tpsData.Value,
                IntakeAirTemp = iatData.Value,
                AirFuelRatio = afrData.Value,
                // Calculate acceleration from speed delta
                Acceleration = CalculateAcceleration(speedData.Value)
            };

            if (_isLogging)
            {
                UpdateSpeedRangeTiming(data);
            }

            return data;
        }

        public async Task<DragSessionData> GetSessionDataAsync()
        {
            List<PerformanceData> data;
            lock (_lockObject)
            {
                data = _sessionData.ToList();
            }

            if (data.Count == 0)
                return null;

            // Calculate drag run metrics
            return new DragSessionData
            {
                ReactionTime = CalculateReactionTime(data),
                SixtyFoot = GetTimeToDistance(data, 60),
                EighthMile = GetTimeToDistance(data, 660),
                EighthMileSpeed = GetSpeedAtDistance(data, 660),
                ThousandFoot = GetTimeToDistance(data, 1000),
                QuarterMile = GetTimeToDistance(data, 1320),
                QuarterMileSpeed = GetSpeedAtDistance(data, 1320),
                PeakPower = data.Max(d => CalculatePower(d.TorqueActual, d.EngineRPM)),
                PeakTorque = data.Max(d => d.TorqueActual),
                BestVE = data.Max(d => d.VolumetricEfficiency),
                // Add speed range times
                sixty_to_130 = _60to130,
                hundred_to_150 = _100to150
            };
        }

        public Task SetDisplayBrightnessAsync(byte level)
        {
            return _obdService.SendCommandAsync($"AT L{level:X2}");
        }

        public Task SetCanProtocolAsync(byte canBus, uint baudRate)
        {
            return _obdService.SendCommandAsync($"AT SP{canBus:X2}");
        }

        public Task<bool> ConfigureLoggingAsync(uint intervalMs)
        {
            // Configure logging interval
            return Task.FromResult(true);
        }

        private float CalculateVE(float maf, float rpm, float map, float iat)
        {
            const float engineDisplacement = 5.0f; // Liters
            const float airRConstant = 287.058f;

            var airDensity = (map * 1000) / (airRConstant * (iat + 273.15f));
            var theoreticalAirflow = (engineDisplacement * rpm * airDensity) / 120.0f;
            var actualAirflow = maf;

            return (actualAirflow / theoreticalAirflow) * 100.0f;
        }

        private float ScaleMAF(float rawMaf, float iat, float baro)
        {
            var tempFactor = (float)Math.Sqrt((iat + 273.15) / 298.15);
            var pressureFactor = baro / 101.325f;
            return rawMaf * tempFactor * pressureFactor;
        }

        private float CalculateTorque(float maf, float rpm, float sparkAdvance)
        {
            const float airFuelRatio = 14.7f;
            const float thermalEfficiency = 0.35f;
            const float torqueFactor = 120.0f * thermalEfficiency / (2 * (float)Math.PI);

            var timingFactor = 1.0f + (sparkAdvance - 20.0f) * 0.003f;

            return (maf * airFuelRatio * torqueFactor * timingFactor) / rpm;
        }

        private float CalculateAcceleration(float currentSpeed)
        {
            // Implementation for acceleration calculation based on speed delta
            return 0.0f; // Placeholder
        }

        private float CalculateReactionTime(List<PerformanceData> data)
        {
            // Implementation for reaction time calculation
            return 0.0f; // Placeholder
        }

        private float GetTimeToDistance(List<PerformanceData> data, float distance)
        {
            // Implementation for time to distance calculation
            return 0.0f; // Placeholder
        }

        private float GetSpeedAtDistance(List<PerformanceData> data, float distance)
        {
            // Implementation for speed at distance calculation
            return 0.0f; // Placeholder
        }

        private float CalculatePower(float torque, float rpm)
        {
            return (torque * rpm) / 5252.0f; // HP calculation
        }

        private void UpdateSpeedRangeTiming(PerformanceData data)
        {
            // Update 60-130 timing
            if (!_60to130.completed)
            {
                if (!_60to130.in_progress && data.VehicleSpeed >= _60to130.start_speed)
                {
                    _60to130.in_progress = true;
                    _60to130.start_timestamp = data.timestamp_us;
                }
                else if (_60to130.in_progress && data.VehicleSpeed >= _60to130.end_speed)
                {
                    _60to130.elapsed_time = (data.timestamp_us - _60to130.start_timestamp) / 1000000.0f;
                    _60to130.completed = true;
                }
            }

            // Update 100-150 timing
            if (!_100to150.completed)
            {
                if (!_100to150.in_progress && data.VehicleSpeed >= _100to150.start_speed)
                {
                    _100to150.in_progress = true;
                    _100to150.start_timestamp = data.timestamp_us;
                }
                else if (_100to150.in_progress && data.VehicleSpeed >= _100to150.end_speed)
                {
                    _100to150.elapsed_time = (data.timestamp_us - _100to150.start_timestamp) / 1000000.0f;
                    _100to150.completed = true;
                }
            }
        }
    }
}
