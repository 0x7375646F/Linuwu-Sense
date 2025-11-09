# Build script for Acer Sense Windows 11

Write-Host "Building Acer Sense for Windows 11..." -ForegroundColor Cyan

# Check if .NET SDK is installed
$dotnetVersion = dotnet --version 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: .NET SDK not found!" -ForegroundColor Red
    Write-Host "Please install .NET 8.0 SDK from: https://dotnet.microsoft.com/download/dotnet/8.0" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found .NET SDK version: $dotnetVersion" -ForegroundColor Green

# Restore dependencies
Write-Host "`nRestoring NuGet packages..." -ForegroundColor Cyan
dotnet restore AcerSense.sln
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to restore packages!" -ForegroundColor Red
    exit 1
}

# Build Release configuration
Write-Host "`nBuilding Release configuration..." -ForegroundColor Cyan
dotnet build AcerSense.sln --configuration Release --no-restore
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`nBuild completed successfully!" -ForegroundColor Green
Write-Host "Output directory: AcerSense.UI\bin\Release\net8.0-windows" -ForegroundColor Cyan
Write-Host "`nTo run the application:" -ForegroundColor Yellow
Write-Host "  1. Navigate to the output directory" -ForegroundColor Yellow
Write-Host "  2. Right-click AcerSense.UI.exe" -ForegroundColor Yellow
Write-Host "  3. Select 'Run as Administrator'" -ForegroundColor Yellow
