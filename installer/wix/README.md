# WiX Installer

This directory contains the Windows MSI packaging entrypoint for NeurolingsCE.

## Workflow

1. Build the application in Release mode.
2. Stage a clean distributable directory with:

```powershell
powershell -ExecutionPolicy Bypass -File .\src\tools\package-windows-bin.ps1
```

3. Generate or build the MSI with:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\wix\build-msi.ps1
```

4. Optionally build the WiX bootstrapper bundle that chains `vc_redist.x64.exe` and the MSI:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\wix\build-bundle.ps1
```

## Notes

- The script reads metadata from `VERSION.txt`.
- By default it looks for the newest packaged directory under `out/package/`.
- It generates temporary `.wxs` sources under `out/installer/wix/`.
- It targets WiX Toolset 7 (`wix build`) and auto-detects `C:\Program Files\WiX Toolset v7.0\bin\wix.exe` when available.
- Installer assets such as the icon live under `installer/wix/assets/`.
- The installer license text is stored in `installer/wix/assets/license_en-US.rtf` and is shown in English in both the MSI UI and the bootstrapper bundle.
- `build-msi.ps1` outputs a single English MSI named after the packaged directory, for example `NeurolingsCE_windows_x86_64_v0.3.0a.msi`.
- `build-bundle.ps1` outputs a single English bootstrapper bundle named `NeurolingsCE_windows_x86_64_v0.3.0a-setup.exe`.
- `build-msi.ps1` adds an options page with three default-enabled checkboxes: create a desktop shortcut, create a Start menu shortcut, and install the bundled NeurolingsCE companion skill files.
- The packaged `neurolingsce-skill` and `neurolingsce-companion` files are installed under the application directory when the companion skill option stays enabled.
- `build-msi.ps1` also keeps the optional launch checkbox on exit and the VC++ redistributable custom action when `vc_redist.x64.exe` exists in the packaged directory.
- `build-bundle.ps1` creates a bootstrapper `.exe` that installs `vc_redist.x64.exe` before the MSI.
- If WiX is not installed yet, you can still preview the generated WiX sources with:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\wix\build-msi.ps1 -GenerateOnly
```
