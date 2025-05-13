using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace OBD2Tool.ViewModels
{
    public partial class SafetySettingsViewModel : ObservableObject
    {
        private readonly IPerformanceService _perfService;

        [ObservableProperty]
        private bool _passiveModeEnabled = true;  // Default to passive mode for safety

        [ObservableProperty]
        private bool _safetyChecksEnabled = true;

        [ObservableProperty]
        private bool _dataValidationEnabled = true;

        [ObservableProperty]
        private float _rpmLimit = 8000;

        [ObservableProperty]
        private float _boostLimit = 30;

        [ObservableProperty]
        private float _egtLimit = 1600;

        [ObservableProperty]
        private float _coolantTempLimit = 230;

        [ObservableProperty]
        private float _minOilPressure = 10;

        [ObservableProperty]
        private uint _commandTimeoutMs = 1000;

        [ObservableProperty]
        private bool _blockActiveCommands = true;

        [ObservableProperty]
        private bool _logAllCommands = true;

        [ObservableProperty]
        private bool _validateResponses = true;

        // Logging settings
        [ObservableProperty]
        private bool _autoExportCsv = true;

        [ObservableProperty]
        private bool _includeTimestamps = true;

        [ObservableProperty]
        private bool _compressLogs = false;

        [ObservableProperty]
        private uint _bufferSize = 1000000;  // 1M samples

        [ObservableProperty]
        private uint _flushInterval = 5000;  // 5 seconds

        [ObservableProperty]
        private bool _highPrecision = true;

        [ObservableProperty]
        private bool _autoSaveEnabled = true;

        [ObservableProperty]
        private uint _autoSaveInterval = 30000;  // 30 seconds

        public SafetySettingsViewModel(IPerformanceService perfService)
        {
            _perfService = perfService;
        }

        [RelayCommand]
        private async Task SaveSettings()
        {
            var safetyConfig = new SafetyMonitor
            {
                passive_mode_enabled = PassiveModeEnabled,
                safety_checks_enabled = SafetyChecksEnabled,
                data_validation_enabled = DataValidationEnabled,
                rpm_limit = RpmLimit,
                boost_limit = BoostLimit,
                egt_limit = EgtLimit,
                coolant_temp_limit = CoolantTempLimit,
                min_oil_pressure = MinOilPressure,
                command_timeout_ms = CommandTimeoutMs,
                safety_config = new SafetyConfig
                {
                    block_active_commands = BlockActiveCommands,
                    log_all_commands = LogAllCommands,
                    validate_responses = ValidateResponses
                }
            };

            var logConfig = new LogConfig
            {
                auto_export_csv = AutoExportCsv,
                include_timestamps = IncludeTimestamps,
                compress_logs = CompressLogs,
                buffer_config = new BufferConfig
                {
                    buffer_size = BufferSize,
                    flush_interval = FlushInterval,
                    high_precision = HighPrecision
                },
                auto_save = new AutoSaveConfig
                {
                    enabled = AutoSaveEnabled,
                    interval_ms = AutoSaveInterval,
                    backup_dir = Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
                        "OBD2Logs",
                        "Backups"
                    )
                }
            };

            await _perfService.UpdateSafetySettings(safetyConfig);
            await _perfService.UpdateLoggingSettings(logConfig);
        }

        [RelayCommand]
        private void RestoreDefaults()
        {
            // Reset all settings to their safe defaults
            PassiveModeEnabled = true;
            SafetyChecksEnabled = true;
            DataValidationEnabled = true;
            RpmLimit = 8000;
            BoostLimit = 30;
            EgtLimit = 1600;
            CoolantTempLimit = 230;
            MinOilPressure = 10;
            CommandTimeoutMs = 1000;
            BlockActiveCommands = true;
            LogAllCommands = true;
            ValidateResponses = true;
            
            // Logging defaults
            AutoExportCsv = true;
            IncludeTimestamps = true;
            CompressLogs = false;
            BufferSize = 1000000;
            FlushInterval = 5000;
            HighPrecision = true;
            AutoSaveEnabled = true;
            AutoSaveInterval = 30000;
        }
    }
}
