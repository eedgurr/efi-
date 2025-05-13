using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using OBD2Tool.Services;

namespace OBD2Tool.ViewModels
{
    public partial class EngineHealthViewModel : ObservableObject
    {
        private readonly IAdvancedDataLoggerService _loggerService;
        private readonly Timer _updateTimer;

        [ObservableProperty]
        private float _shortTermFuelTrim;

        [ObservableProperty]
        private float _longTermFuelTrim;

        [ObservableProperty]
        private float _volumetricEfficiency;

        [ObservableProperty]
        private float _combustionEfficiency;

        [ObservableProperty]
        private float _knockCount;

        [ObservableProperty]
        private float _timingCorrection;

        [ObservableProperty]
        private bool _isLogging;

        [ObservableProperty]
        private string _sessionDescription;

        [ObservableProperty]
        private string _selectedLogFormat = "xdf";

        public ObservableCollection<LogSession> Sessions { get; } = new();
        public ObservableCollection<ChartDataPoint> EfficiencyHistory { get; } = new();

        public EngineHealthViewModel(IAdvancedDataLoggerService loggerService)
        {
            _loggerService = loggerService;
            _updateTimer = new Timer(UpdateHealthMetrics, null, 0, 100); // 10Hz updates
        }

        private async void UpdateHealthMetrics(object state)
        {
            try
            {
                var metrics = await _loggerService.GetRealTimeHealthMetrics();
                
                MainThread.BeginInvokeOnMainThread(() =>
                {
                    ShortTermFuelTrim = metrics["ShortTermFuelTrim"];
                    LongTermFuelTrim = metrics["LongTermFuelTrim"];
                    VolumetricEfficiency = metrics["VolumetricEfficiency"];
                    CombustionEfficiency = metrics["CombustionEfficiency"];
                    KnockCount = metrics["KnockCount"];
                    TimingCorrection = metrics["TimingCorrection"];

                    // Update efficiency history chart
                    EfficiencyHistory.Add(new ChartDataPoint 
                    { 
                        Timestamp = DateTime.Now,
                        Value = VolumetricEfficiency 
                    });

                    if (EfficiencyHistory.Count > 100) // Keep last 100 points
                        EfficiencyHistory.RemoveAt(0);
                });
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Health metrics update error: {ex.Message}");
            }
        }

        [RelayCommand]
        private async Task ToggleLogging()
        {
            if (!IsLogging)
            {
                if (string.IsNullOrWhiteSpace(SessionDescription))
                {
                    SessionDescription = $"Session {DateTime.Now:yyyy-MM-dd HH:mm:ss}";
                }

                IsLogging = await _loggerService.StartLoggingSession(SessionDescription);
            }
            else
            {
                await _loggerService.StopLoggingSession();
                IsLogging = false;
                
                // Refresh sessions list
                var sessions = await _loggerService.GetAllSessions();
                Sessions.Clear();
                foreach (var session in sessions)
                {
                    Sessions.Add(session);
                }
            }
        }

        [RelayCommand]
        private async Task ExportSession(LogSession session)
        {
            try
            {
                await _loggerService.ExportSession(session.Id, SelectedLogFormat);
                await Shell.Current.DisplayAlert("Success", 
                    $"Session exported successfully in {SelectedLogFormat} format", "OK");
            }
            catch (Exception ex)
            {
                await Shell.Current.DisplayAlert("Error", 
                    $"Failed to export session: {ex.Message}", "OK");
            }
        }

        [RelayCommand]
        private async Task CompareSession(LogSession session)
        {
            // Get the most recent session to compare with
            var currentSession = await _loggerService.GetCurrentSession();
            if (currentSession == null) return;

            var analysis = await _loggerService.CompareSessionsHealth(
                session.Id, currentSession.Id);

            await Shell.Current.Navigation.PushAsync(
                new EngineHealthComparisonPage(analysis));
        }
    }
}
