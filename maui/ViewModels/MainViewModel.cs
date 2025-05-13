using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using OBD2Tool.Services;
using Plugin.BLE.Abstractions.Contracts;

namespace OBD2Tool.ViewModels
{
    public partial class MainViewModel : ObservableObject
    {
        private readonly IBluetoothService _bluetoothService;
        private readonly IOBDService _obdService;

        [ObservableProperty]
        private bool _isScanning;

        [ObservableProperty]
        private bool _isConnected;

        [ObservableProperty]
        private string _statusMessage;

        public ObservableCollection<IDevice> DiscoveredDevices => _bluetoothService.DiscoveredDevices;

        public MainViewModel(IBluetoothService bluetoothService, IOBDService obdService)
        {
            _bluetoothService = bluetoothService;
            _obdService = obdService;

            _bluetoothService.DeviceDiscovered += (s, e) => 
                StatusMessage = $"Discovered device: {e.Device.Name}";
            
            _bluetoothService.DeviceConnected += (s, e) => 
            {
                IsConnected = true;
                StatusMessage = $"Connected to {e.Device.Name}";
            };

            _bluetoothService.DeviceDisconnected += (s, e) => 
            {
                IsConnected = false;
                StatusMessage = "Disconnected";
            };
        }

        [RelayCommand]
        private async Task StartScanningAsync()
        {
            try
            {
                if (!await _bluetoothService.RequestLocationPermissionAsync() ||
                    !await _bluetoothService.RequestBluetoothPermissionAsync())
                {
                    StatusMessage = "Missing required permissions";
                    return;
                }

                IsScanning = true;
                StatusMessage = "Scanning for devices...";
                await _bluetoothService.StartScanningAsync();
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error scanning: {ex.Message}";
            }
        }

        [RelayCommand]
        private async Task StopScanningAsync()
        {
            IsScanning = false;
            await _bluetoothService.StopScanningAsync();
            StatusMessage = "Scanning stopped";
        }

        [RelayCommand]
        private async Task ConnectToDeviceAsync(IDevice device)
        {
            try
            {
                StatusMessage = $"Connecting to {device.Name}...";
                await _bluetoothService.ConnectToDeviceAsync(device);
            }
            catch (Exception ex)
            {
                StatusMessage = $"Connection error: {ex.Message}";
            }
        }

        [RelayCommand]
        private async Task DisconnectAsync()
        {
            try
            {
                await _bluetoothService.DisconnectAsync();
                StatusMessage = "Disconnected";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Disconnection error: {ex.Message}";
            }
        }
    }
}
