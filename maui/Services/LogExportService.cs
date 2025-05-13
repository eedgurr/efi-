using System;
using System.IO;
using System.Text;
using CsvHelper;
using System.Globalization;
using OBD2Tool.Models;

namespace OBD2Tool.Services
{
    public class LogExportService
    {
        private readonly string _baseLogPath;
        private readonly IPerformanceService _perfService;

        public LogExportService(IPerformanceService perfService)
        {
            _perfService = perfService;
            _baseLogPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.Downloads),
                "OBD2Logs"
            );
            Directory.CreateDirectory(_baseLogPath);
        }

        public async Task<string> ExportSessionToCSV(string sessionId, List<PerformanceData> sessionData)
        {
            var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            var fileName = $"DragRun_{sessionId}_{timestamp}.csv";
            var filePath = Path.Combine(_baseLogPath, fileName);

            try
            {
                using var writer = new StreamWriter(filePath);
                using var csv = new CsvWriter(writer, CultureInfo.InvariantCulture);

                // Write headers
                csv.WriteField("Timestamp");
                csv.WriteField("Time (ms)");
                
                // Engine Data
                csv.WriteField("RPM");
                csv.WriteField("Load (%)");
                csv.WriteField("Throttle (%)");
                csv.WriteField("MAF (g/s)");
                csv.WriteField("MAP (kPa)");
                csv.WriteField("Boost (PSI)");
                csv.WriteField("AFR");
                csv.WriteField("Timing (deg)");
                csv.WriteField("Engine Power (HP)");
                csv.WriteField("Engine Torque (lb-ft)");
                
                // Temperatures
                csv.WriteField("Coolant (°F)");
                csv.WriteField("Oil (°F)");
                csv.WriteField("Intake (°F)");
                csv.WriteField("Trans (°F)");
                foreach (int i in Enumerable.Range(1, 8))
                {
                    csv.WriteField($"EGT Cyl{i} (°F)");
                }
                
                // Pressures
                csv.WriteField("Oil Pressure (PSI)");
                csv.WriteField("Fuel Pressure (PSI)");
                csv.WriteField("Brake Pressure (PSI)");
                foreach (int i in Enumerable.Range(1, 4))
                {
                    csv.WriteField($"Tire{i} Pressure (PSI)");
                    csv.WriteField($"Tire{i} Temp (°F)");
                }
                
                // Performance
                csv.WriteField("Vehicle Speed (MPH)");
                csv.WriteField("Acceleration (G)");
                csv.WriteField("Current Gear");
                csv.WriteField("Gear Ratio");
                csv.WriteField("Wheel Speed FL (MPH)");
                csv.WriteField("Wheel Speed FR (MPH)");
                csv.WriteField("Wheel Speed RL (MPH)");
                csv.WriteField("Wheel Speed RR (MPH)");
                
                // Knock Data
                foreach (int i in Enumerable.Range(1, 8))
                {
                    csv.WriteField($"Knock Cyl{i}");
                }
                
                // Safety Status
                csv.WriteField("Safety Status");
                csv.WriteField("Warning Flags");
                csv.WriteField("Warning Message");
                csv.WriteField("Safety Margin (%)");
                
                csv.NextRecord();

                // Write data rows
                var startTime = sessionData.First().TimestampUs;
                foreach (var data in sessionData)
                {
                    csv.WriteField(DateTimeOffset.FromUnixTimeMilliseconds(data.TimestampUs / 1000).ToString("O"));
                    csv.WriteField((data.TimestampUs - startTime) / 1000.0); // Time in ms
                    
                    // Engine Data
                    csv.WriteField(data.EngineRPM);
                    csv.WriteField(data.Load);
                    csv.WriteField(data.ThrottlePosition);
                    csv.WriteField(data.MafScaled);
                    csv.WriteField(data.MapPressure);
                    csv.WriteField(data.BoostActual);
                    csv.WriteField(data.AirFuelRatio);
                    csv.WriteField(data.TimingAdvance);
                    csv.WriteField(data.CurrentPower);
                    csv.WriteField(data.TorqueActual);
                    
                    // Temperatures
                    csv.WriteField(data.SensorData.CoolantTemp);
                    csv.WriteField(data.SensorData.OilTemp);
                    csv.WriteField(data.IntakeAirTemp);
                    csv.WriteField(data.SensorData.TransTemp);
                    foreach (var egt in data.SensorData.Egt)
                    {
                        csv.WriteField(egt);
                    }
                    
                    // Pressures
                    csv.WriteField(data.SensorData.OilPressure);
                    csv.WriteField(data.SensorData.FuelPressure);
                    csv.WriteField(data.SensorData.BrakePressure);
                    for (int i = 0; i < 4; i++)
                    {
                        csv.WriteField(data.SensorData.TirePressure[i]);
                        csv.WriteField(data.SensorData.TireTemp[i]);
                    }
                    
                    // Performance
                    csv.WriteField(data.VehicleSpeed);
                    csv.WriteField(data.Acceleration);
                    csv.WriteField(data.CurrentGear);
                    csv.WriteField(data.GearRatio);
                    foreach (var wheelSpeed in data.WheelSpeed)
                    {
                        csv.WriteField(wheelSpeed);
                    }
                    
                    // Knock Data
                    foreach (var knock in data.SensorData.KnockLevel)
                    {
                        csv.WriteField(knock);
                    }
                    
                    // Safety Status
                    csv.WriteField(data.SafetyStatus.InSafeRange);
                    csv.WriteField(data.SafetyStatus.WarningFlags);
                    csv.WriteField(data.SafetyStatus.WarningMessage);
                    csv.WriteField(data.SafetyStatus.SafetyMargin);
                    
                    csv.NextRecord();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error exporting CSV: {ex.Message}");
                throw;
            }

            return filePath;
        }

        public async Task<string> ExportMetaDataToCSV(string sessionId, DragSessionData sessionData)
        {
            var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            var fileName = $"DragRun_{sessionId}_{timestamp}_meta.csv";
            var filePath = Path.Combine(_baseLogPath, fileName);

            try
            {
                using var writer = new StreamWriter(filePath);
                using var csv = new CsvWriter(writer, CultureInfo.InvariantCulture);

                // Write session metadata
                csv.WriteField("Metric");
                csv.WriteField("Value");
                csv.NextRecord();

                csv.WriteField("Session ID");
                csv.WriteField(sessionId);
                csv.NextRecord();

                csv.WriteField("Date/Time");
                csv.WriteField(DateTime.Now.ToString("O"));
                csv.NextRecord();

                csv.WriteField("Reaction Time");
                csv.WriteField(sessionData.ReactionTime);
                csv.NextRecord();

                csv.WriteField("60ft Time");
                csv.WriteField(sessionData.SixtyFoot);
                csv.NextRecord();

                csv.WriteField("330ft Time");
                csv.WriteField(sessionData.ThreeThirtyFoot);
                csv.NextRecord();

                csv.WriteField("1/8 Mile Time");
                csv.WriteField(sessionData.EighthMile);
                csv.NextRecord();

                csv.WriteField("1/8 Mile Speed");
                csv.WriteField(sessionData.EighthMileSpeed);
                csv.NextRecord();

                csv.WriteField("1000ft Time");
                csv.WriteField(sessionData.ThousandFoot);
                csv.NextRecord();

                csv.WriteField("1/4 Mile Time");
                csv.WriteField(sessionData.QuarterMile);
                csv.NextRecord();

                csv.WriteField("1/4 Mile Speed");
                csv.WriteField(sessionData.QuarterMileSpeed);
                csv.NextRecord();

                csv.WriteField("Peak Power");
                csv.WriteField(sessionData.PeakPower);
                csv.NextRecord();

                csv.WriteField("Peak Torque");
                csv.WriteField(sessionData.PeakTorque);
                csv.NextRecord();

                csv.WriteField("Best 60ft");
                csv.WriteField(sessionData.BestSixtyFoot);
                csv.NextRecord();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error exporting meta CSV: {ex.Message}");
                throw;
            }

            return filePath;
        }
    }
}
