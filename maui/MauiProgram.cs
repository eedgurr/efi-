using OBD2Tool.Services;
using OBD2Tool.Views;

namespace OBD2Tool;

public static class MauiProgram
{
    public static MauiApp CreateMauiApp()
    {
        var builder = MauiApp.CreateBuilder();
        builder
            .UseMauiApp<App>()
            .UseMauiCommunityToolkit()
            .ConfigureFonts(fonts =>
            {
                fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
                fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
            });

        // Register services
        builder.Services.AddSingleton<IBluetoothService, BluetoothService>();
        builder.Services.AddSingleton<IOBDService, OBDService>();
        builder.Services.AddSingleton<IDTCService, DTCService>();
        
        // Register views and view models
        builder.Services.AddTransient<MainPage>();
        builder.Services.AddTransient<MainViewModel>();
        builder.Services.AddTransient<DiagnosticsPage>();
        builder.Services.AddTransient<DiagnosticsViewModel>();
        builder.Services.AddTransient<RealTimeDataPage>();
        builder.Services.AddTransient<RealTimeDataViewModel>();
        builder.Services.AddTransient<DTCPage>();
        builder.Services.AddTransient<DTCViewModel>();
        builder.Services.AddTransient<SettingsPage>();
        builder.Services.AddTransient<SettingsViewModel>();

        // Configure logging
        builder.Logging.AddDebug();

        return builder.Build();
    }
}
