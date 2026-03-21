Import("env")
import subprocess
import time

def close_serial_monitor(source, target, env):
    port = "COM30"
    print(f"\n[pre_upload] Releasing {port}...")

    # Kill any python process running 'device monitor' (pio device monitor)
    subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "Get-CimInstance Win32_Process | "
         "Where-Object { $_.CommandLine -like '*device*monitor*' } | "
         "ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"],
        capture_output=True
    )

    # Also kill any python process that has COM30 open
    subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "Get-CimInstance Win32_Process | "
         "Where-Object { $_.CommandLine -like '*COM30*' } | "
         "ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"],
        capture_output=True
    )

    time.sleep(0.75)
    print(f"[pre_upload] {port} released.")

env.AddPreAction("upload", close_serial_monitor)
