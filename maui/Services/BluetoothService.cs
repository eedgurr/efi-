using System;
using Plugin.BLE;
using Plugin.BLE.Abstractions.Contracts;
using Plugin.BLE.Abstractions.EventArgs;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace OBD2Tool.Services
{
    public class BluetoothService : IBluetoothService
    {
        private readonly IBluetoothLE _bluetoothLE;
        private readonly IAdapter _adapter;
        private IDevice _connectedDevice;
        private bool _isScanning;

        public event EventHandler<DeviceEventArgs> DeviceDiscovered;
        public event EventHandler<DeviceEventArgs> DeviceConnected;
        public event EventHandler<DeviceEventArgs> DeviceDisconnected;

        public ObservableCollection<IDevice> DiscoveredDevices { get; }

        public BluetoothService()
        {
            _bluetoothLE = CrossBluetoothLE.Current;
            _adapter = CrossBluetoothLE.Current.Adapter;
            DiscoveredDevices = new ObservableCollection<IDevice>();

            _adapter.DeviceDiscovered += OnDeviceDiscovered;
            _adapter.DeviceConnected += OnDeviceConnected;
            _adapter.DeviceDisconnected += OnDeviceDisconnected;
        }

        public async Task StartScanningAsync()
        {
            if (_isScanning)
                return;

            DiscoveredDevices.Clear();
            _isScanning = true;

            try
            {
                await _adapter.StartScanningForDevicesAsync();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error starting scan: {ex.Message}");
                throw;
            }
        }

        public async Task StopScanningAsync()
        {
            if (!_isScanning)
                return;

            _isScanning = false;
            await _adapter.StopScanningForDevicesAsync();
        }

        public async Task ConnectToDeviceAsync(IDevice device)
        {
            try
            {
                await _adapter.ConnectToDeviceAsync(device);
                _connectedDevice = device;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error connecting to device: {ex.Message}");
                throw;
            }
        }

        public async Task DisconnectAsync()
        {
            if (_connectedDevice != null)
            {
                await _adapter.DisconnectDeviceAsync(_connectedDevice);
                _connectedDevice = null;
            }
        }

        private void OnDeviceDiscovered(object sender, DeviceEventArgs args)
        {
            if (!DiscoveredDevices.Contains(args.Device))
            {
                DiscoveredDevices.Add(args.Device);
                DeviceDiscovered?.Invoke(this, args);
            }
        }

        private void OnDeviceConnected(object sender, DeviceEventArgs args)
        {
            DeviceConnected?.Invoke(this, args);
        }

        private void OnDeviceDisconnected(object sender, DeviceEventArgs args)
        {
            DeviceDisconnected?.Invoke(this, args);
        }

        public async Task<bool> RequestLocationPermissionAsync()
        {
            var status = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();
            return status == PermissionStatus.Granted;
        }

        public async Task<bool> RequestBluetoothPermissionAsync()
        {
            var status = await Permissions.RequestAsync<Permissions.Bluetooth>();
            return status == PermissionStatus.Granted;
        }
    }
}
