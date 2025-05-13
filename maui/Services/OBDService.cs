using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using OBD2Tool.Models;
using System.IO.Ports;
using System.Threading;

namespace OBD2Tool.Services
{
    public class OBDService : IOBDService
    {
        private SerialPort _port;
        private readonly object _lock = new object();
        private bool _isConnected;
        private bool _isMonitoring;
        private CancellationTokenSource _monitoringCts;

        public event EventHandler<OBDDataEventArgs> DataReceived;
        public event EventHandler<OBDErrorEventArgs> ErrorOccurred;

        public bool IsConnected => _isConnected;
        public ObservableCollection<PIDData> SupportedPIDs { get; }

        public OBDService()
        {
            SupportedPIDs = new ObservableCollection<PIDData>();
        }

        public async Task<bool> ConnectAsync(string portName, int baudRate = 38400)
        {
            if (_isConnected)
                return true;

            try
            {
                _port = new SerialPort(portName, baudRate)
                {
                    ReadTimeout = 2000,
                    WriteTimeout = 2000
                };

                _port.Open();
                await InitializeOBDDeviceAsync();
                _isConnected = true;
                return true;
            }
            catch (Exception ex)
            {
                ErrorOccurred?.Invoke(this, new OBDErrorEventArgs(ex.Message));
                return false;
            }
        }

        public async Task DisconnectAsync()
        {
            if (!_isConnected)
                return;

            await StopMonitoringAsync();

            lock (_lock)
            {
                _port?.Close();
                _port?.Dispose();
                _port = null;
                _isConnected = false;
            }
        }

        private async Task InitializeOBDDeviceAsync()
        {
            // Reset device
            await SendCommandAsync("ATZ");
            await Task.Delay(1000); // Wait for device reset

            // Configure device
            await SendCommandAsync("ATE0"); // Echo off
            await SendCommandAsync("ATL0"); // Linefeeds off
            await SendCommandAsync("ATH0"); // Headers off
            await SendCommandAsync("ATS0"); // Spaces off
            await SendCommandAsync("ATSP0"); // Auto protocol

            // Get supported PIDs
            await GetSupportedPIDsAsync();
        }

        public async Task StartMonitoringAsync(int interval = 100)
        {
            if (_isMonitoring || !_isConnected)
                return;

            _isMonitoring = true;
            _monitoringCts = new CancellationTokenSource();

            await Task.Run(async () =>
            {
                while (!_monitoringCts.Token.IsCancellationRequested)
                {
                    try
                    {
                        foreach (var pid in SupportedPIDs)
                        {
                            var data = await RequestPIDAsync(pid.ID);
                            if (data != null)
                            {
                                DataReceived?.Invoke(this, new OBDDataEventArgs(data));
                            }
                        }
                        await Task.Delay(interval, _monitoringCts.Token);
                    }
                    catch (OperationCanceledException)
                    {
                        break;
                    }
                    catch (Exception ex)
                    {
                        ErrorOccurred?.Invoke(this, new OBDErrorEventArgs(ex.Message));
                    }
                }
            }, _monitoringCts.Token);
        }

        public async Task StopMonitoringAsync()
        {
            if (!_isMonitoring)
                return;

            _monitoringCts?.Cancel();
            _isMonitoring = false;
            await Task.Delay(100); // Allow monitoring loop to stop
        }

        private async Task GetSupportedPIDsAsync()
        {
            SupportedPIDs.Clear();

            // Request supported PIDs (mode 01)
            var response = await SendCommandAsync("0100");
            if (string.IsNullOrEmpty(response))
                return;

            // Parse supported PIDs
            var pidData = ParseSupportedPIDs(response);
            foreach (var pid in pidData)
            {
                SupportedPIDs.Add(pid);
            }
        }

        private async Task<OBDData> RequestPIDAsync(string pid)
        {
            var command = $"01{pid}";
            var response = await SendCommandAsync(command);
            return ParseOBDResponse(pid, response);
        }

        private async Task<string> SendCommandAsync(string command)
        {
            if (!_isConnected)
                throw new InvalidOperationException("Not connected to OBD device");

            lock (_lock)
            {
                _port.WriteLine(command);
                return _port.ReadLine().Trim();
            }
        }

        private List<PIDData> ParseSupportedPIDs(string response)
        {
            var pids = new List<PIDData>();
            // Implementation of PID support parsing
            return pids;
        }

        private OBDData ParseOBDResponse(string pid, string response)
        {
            // Implementation of OBD response parsing
            return new OBDData();
        }
    }

    public class OBDDataEventArgs : EventArgs
    {
        public OBDData Data { get; }
        public OBDDataEventArgs(OBDData data) => Data = data;
    }

    public class OBDErrorEventArgs : EventArgs
    {
        public string Message { get; }
        public OBDErrorEventArgs(string message) => Message = message;
    }
}
