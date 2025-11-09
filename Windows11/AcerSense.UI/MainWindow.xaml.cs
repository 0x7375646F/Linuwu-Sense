using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using AcerSense.Core.Detection;
using AcerSense.Core.Models;
using AcerSense.Core.Services;

namespace AcerSense.UI;

public partial class MainWindow
{
    private AcerControlService? _controlService;
    private DispatcherTimer? _refreshTimer;

    public MainWindow()
    {
        InitializeComponent();
        Loaded += MainWindow_Loaded;
        Closing += MainWindow_Closing;
    }

    private void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        InitializeService();
        StartRefreshTimer();
    }

    private void MainWindow_Closing(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        _refreshTimer?.Stop();
        _controlService?.Dispose();
    }

    private void InitializeService()
    {
        try
        {
            _controlService = new AcerControlService();

            if (!_controlService.Initialize())
            {
                MessageBox.Show(
                    "Failed to initialize Acer hardware control.\n\n" +
                    "Please ensure:\n" +
                    "1. You are running on a supported Acer laptop\n" +
                    "2. The application is running as Administrator\n" +
                    "3. WMI service is enabled",
                    "Initialization Error",
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);

                StatusIndicator.Text = "🔴";
                StatusIndicator.ToolTip = "Not Connected";
                return;
            }

            // Update UI with device info
            DeviceNameText.Text = _controlService.DeviceQuirk.ModelName;
            AboutDeviceText.Text = $"{_controlService.DeviceQuirk.Manufacturer} {_controlService.DeviceQuirk.ModelName}";

            // Populate features list
            var features = DeviceDetector.GetAvailableFeatures(_controlService.DeviceQuirk);
            FeaturesList.ItemsSource = features.Count > 0 ? features : new List<string> { "No features available" };

            // Load current settings
            LoadCurrentSettings();

            StatusIndicator.Text = "🟢";
            StatusIndicator.ToolTip = "Connected";
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                $"Error initializing service: {ex.Message}",
                "Error",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    private void LoadCurrentSettings()
    {
        if (_controlService == null) return;

        try
        {
            // Load performance profile
            var profile = _controlService.GetPerformanceProfile();
            PerformanceProfileCombo.SelectedIndex = (int)profile;

            // Load battery health status
            var (mode, limit) = _controlService.GetBatteryHealthStatus();
            BatteryHealthToggle.IsOn = mode == BatteryMode.HealthMode;
            BatteryLimitSlider.Value = limit;
        }
        catch
        {
            // Ignore errors during initial load
        }
    }

    private void StartRefreshTimer()
    {
        _refreshTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromSeconds(2)
        };
        _refreshTimer.Tick += RefreshTimer_Tick;
        _refreshTimer.Start();
    }

    private void RefreshTimer_Tick(object? sender, EventArgs e)
    {
        UpdateFanSpeeds();
    }

    private void UpdateFanSpeeds()
    {
        if (_controlService == null) return;

        try
        {
            int cpuFanSpeed = _controlService.GetCpuFanSpeed();
            int gpuFanSpeed = _controlService.GetGpuFanSpeed();

            CpuFanSpeedText.Text = $"Speed: {cpuFanSpeed} RPM";
            GpuFanSpeedText.Text = $"Speed: {gpuFanSpeed} RPM";
        }
        catch
        {
            // Ignore refresh errors
        }
    }

    #region Event Handlers

    private void RefreshButton_Click(object sender, RoutedEventArgs e)
    {
        LoadCurrentSettings();
        UpdateFanSpeeds();
    }

    private void PerformanceProfile_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (_controlService == null || PerformanceProfileCombo.SelectedItem == null)
            return;

        var selectedItem = (ComboBoxItem)PerformanceProfileCombo.SelectedItem;
        int profileIndex = int.Parse(selectedItem.Tag.ToString()!);

        _controlService.SetPerformanceProfile((PerformanceProfile)profileIndex);
    }

    private void TurboMode_Toggled(object sender, RoutedEventArgs e)
    {
        if (_controlService == null)
            return;

        if (TurboModeToggle.IsOn)
            _controlService.EnableTurboMode();
        else
            _controlService.DisableTurboMode();
    }

    private void CpuFanSlider_Changed(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (_controlService == null)
            return;

        int percent = (int)CpuFanSlider.Value;
        CpuFanValueText.Text = $"{percent}%";
        _controlService.SetCpuFanSpeed(percent);
    }

    private void GpuFanSlider_Changed(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (_controlService == null)
            return;

        int percent = (int)GpuFanSlider.Value;
        GpuFanValueText.Text = $"{percent}%";
        _controlService.SetGpuFanSpeed(percent);
    }

    private void BatteryHealth_Toggled(object sender, RoutedEventArgs e)
    {
        if (_controlService == null)
            return;

        if (BatteryHealthToggle.IsOn)
        {
            int limit = (int)BatteryLimitSlider.Value;
            _controlService.EnableBatteryHealthMode(limit);
        }
        else
        {
            _controlService.DisableBatteryHealthMode();
        }
    }

    private void BatteryLimit_Changed(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (_controlService == null)
            return;

        int limit = (int)BatteryLimitSlider.Value;
        BatteryLimitText.Text = $"{limit}%";

        if (BatteryHealthToggle.IsOn)
        {
            _controlService.EnableBatteryHealthMode(limit);
        }
    }

    private void StartCalibration_Click(object sender, RoutedEventArgs e)
    {
        if (_controlService == null)
            return;

        var result = MessageBox.Show(
            "Battery calibration will fully charge then discharge your battery.\n" +
            "This process may take several hours.\n\n" +
            "Continue?",
            "Battery Calibration",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);

        if (result == MessageBoxResult.Yes)
        {
            if (_controlService.StartBatteryCalibration())
            {
                MessageBox.Show(
                    "Battery calibration started.\n" +
                    "Keep your laptop plugged in during this process.",
                    "Calibration Started",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show(
                    "Failed to start battery calibration.",
                    "Error",
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }
        }
    }

    #region RGB Keyboard Handlers

    private void Rainbow_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardRainbow();
    }

    private void RedPreset_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardColor(255, 0, 0);
    }

    private void GreenPreset_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardColor(0, 255, 0);
    }

    private void BluePreset_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardColor(0, 0, 255);
    }

    private void WhitePreset_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardColor(255, 255, 255);
    }

    private void OffPreset_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardColor(0, 0, 0);
    }

    private void ApplyZone1_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardZoneColor(
            KeyboardZone.Zone1,
            (byte)Zone1RedSlider.Value,
            (byte)Zone1GreenSlider.Value,
            (byte)Zone1BlueSlider.Value);
    }

    private void ApplyZone2_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardZoneColor(
            KeyboardZone.Zone2,
            (byte)Zone2RedSlider.Value,
            (byte)Zone2GreenSlider.Value,
            (byte)Zone2BlueSlider.Value);
    }

    private void ApplyZone3_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardZoneColor(
            KeyboardZone.Zone3,
            (byte)Zone3RedSlider.Value,
            (byte)Zone3GreenSlider.Value,
            (byte)Zone3BlueSlider.Value);
    }

    private void ApplyZone4_Click(object sender, RoutedEventArgs e)
    {
        _controlService?.SetKeyboardZoneColor(
            KeyboardZone.Zone4,
            (byte)Zone4RedSlider.Value,
            (byte)Zone4GreenSlider.Value,
            (byte)Zone4BlueSlider.Value);
    }

    #endregion

    #endregion
}
