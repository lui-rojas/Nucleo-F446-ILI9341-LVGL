# Setup Guide: Nucleo-F446-ILI9341-LVGL

**Project:** `Nucleo-F446-ILI9341-LVGL`  
**Board:** STM32F446RE (Nucleo-64)  
**Display:** ILI9341 via SPI  
**UI Library:** LVGL v8.3  
**RTOS:** FreeRTOS (via STM32CubeMX)  
**Build System:** Generic Makefile  
**IDE:** VS Code (cross-platform, no proprietary IDE)

This guide documents the complete, verified setup for compiling, flashing, and debugging the project on both **Ubuntu 24.04** and **Windows 11** using a generic, portable Makefile-based workflow.

---

## Table of Contents

1. [Repository & GitHub Setup](#1-repository--github-setup)
2. [Project Structure & Directory Layout](#2-project-structure--directory-layout)
3. [Ubuntu 24.04 — Toolchain & Environment](#3-ubuntu-2404--toolchain--environment)
4. [Windows 11 — Toolchain & Environment](#4-windows-11--toolchain--environment)
5. [VS Code Configuration (Cross-Platform)](#5-vs-code-configuration-cross-platform)
6. [Makefile Configuration](#6-makefile-configuration)
7. [ST-LINK Firmware Upgrade (Windows)](#7-st-link-firmware-upgrade-windows)
8. [Build & Debug Workflow](#8-build--debug-workflow)
9. [Syncing Between Machines (Git Workflow)](#9-syncing-between-machines-git-workflow)

---

## 1. Repository & GitHub Setup

### Repository Name

`Nucleo-F446-ILI9341-LVGL`  
GitHub URL: `https://github.com/lui-rojas/Nucleo-F446-ILI9341-LVGL.git`

### `.gitignore`

Create a `.gitignore` in the project root to exclude all compiled artifacts:

```text
# Build output
build/
Debug/
Release/
bin/
obj/

# Compiled artifacts
*.elf
*.bin
*.hex
*.map
*.lst

# OS and editor noise
.DS_Store
.history/
```

> **Note:** The `.vscode/` folder is **intentionally included** in the repository, as it holds the cross-platform `launch.json` and `tasks.json` configurations needed on both machines.

---

### First Push from Ubuntu

```bash
cd /path/to/Nucleo-F446-ILI9341-LVGL

git init
git add .
git commit -m "Initial commit: Nucleo-F446 with LVGL and ILI9341 SPI setup"
git branch -M main
git remote add origin https://github.com/lui-rojas/Nucleo-F446-ILI9341-LVGL.git
git push -u origin main
```

When prompted for a password, paste your **Personal Access Token (PAT)** — not your GitHub account password.

#### Generating a Personal Access Token (PAT)

1. GitHub → Profile → **Settings** → **Developer Settings**
2. **Personal access tokens** → **Tokens (classic)**
3. **Generate new token (classic)**
4. Select scope: `repo`
5. Copy and store the token immediately — it is shown only once.

---

### Cloning on Windows 11

```powershell
git clone https://github.com/lui-rojas/Nucleo-F446-ILI9341-LVGL.git
```

> Cloning a public repository does not require a token. A PAT is only required on the first `git push` from a new machine.

---

### Adding `.vscode/` to Git (Force-Add)

If `.vscode/` was previously ignored but you want to track it:

**Option A — Remove it from `.gitignore` (recommended):**

1. Open `.gitignore` and delete or comment out the `.vscode/` line.
2. Then add and commit normally:

```bash
git add .vscode/
git commit -m "Add cross-platform VS Code debug and build configurations"
git push
```

**Option B — Force-add without modifying `.gitignore`:**

```bash
git add -f .vscode/launch.json
git add -f .vscode/tasks.json
git commit -m "Force add cross-platform launch and task settings"
git push
```

---

## 2. Project Structure & Directory Layout

```
Nucleo-F446-ILI9341-LVGL/
├── Core/
│   ├── Inc/
│   ├── Src/
│   └── lvgl-release-v8.3/       ← LVGL v8.3 library (headers + sources)
├── Drivers/
│   ├── CMSIS/
│   ├── STM32F4xx_HAL_Driver/
│   └── ILI9341/                 ← ILI9341 SPI driver + font files
├── Middlewares/
│   └── Third_Party/
│       └── FreeRTOS/
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── .gitignore
├── Makefile
└── STM32F446.svd
```

---

## 3. Ubuntu 24.04 — Toolchain & Environment

### Package Installation

```bash
sudo apt update
sudo apt install \
  build-essential \
  gcc-arm-none-eabi \
  binutils-arm-none-eabi \
  libnewlib-arm-none-eabi \
  gdb-arm-none-eabi \
  openocd
```

| Package | Purpose |
|---|---|
| `build-essential` | Base build tools including `make` |
| `gcc-arm-none-eabi` | ARM cross-compiler |
| `binutils-arm-none-eabi` | ARM linker and binary utilities |
| `libnewlib-arm-none-eabi` | C standard library for bare-metal ARM |
| `gdb-arm-none-eabi` | ARM GDB debugger |
| `openocd` | On-chip debugger / flash programmer |

### Git Account Setup (Personal vs. Work)

To switch from a work account to a personal GitHub account:

```bash
# Clear stored work credentials
git config --global --unset credential.helper

# Set personal identity for this project
cd /path/to/Nucleo-F446-ILI9341-LVGL
git config user.name "Your Name"
git config user.email "your-personal-email@example.com"
```

---

### Recommended VS Code Extensions (Ubuntu)

| Extension | Purpose |
|---|---|
| **C/C++** (Microsoft) | IntelliSense, error highlighting |
| **Cortex-Debug** | On-hardware step debugging |
| **Makefile Tools** | Makefile integration in VS Code |

---

## 4. Windows 11 — Toolchain & Environment

### Required Installations

| Tool | Purpose | Notes |
|---|---|---|
| **Git for Windows** | Version control | Includes Git Credential Manager |
| **STM32CubeCLT** | ARM compiler + ST-LINK GDB server | Install to `C:\ST\STM32CubeCLT_1.21.0\` |
| **GNU Make for Windows** | `make.exe` build tool | Download from GnuWin32 or xPack; CLT does **not** include `make` |

> **Important:** STM32CubeCLT on Windows includes `cmake` and `ninja` but does **not** include `make.exe`. A standalone `make.exe` must be downloaded separately.

---

### Installing `make.exe` on Windows

1. Download `make.exe` from [GnuWin32](http://gnuwin32.sourceforge.net/packages/make.htm) or the [xPack GNU Make](https://github.com/xpack-binaries/xpack-windows-build-tools/releases) project.
2. Place `make.exe` inside the CLT compiler bin folder:  
   `C:\ST\STM32CubeCLT_1.21.0\GNU-tools-for-STM32\bin\`
3. Alternatively, add the folder to the Windows **System Environment Variable `PATH`**:
   - Search: **"Edit the system environment variables"**
   - **Environment Variables** → **System Variables** → **Path** → **Edit** → **New**
   - Add: `C:\ST\STM32CubeCLT_1.21.0\GNU-tools-for-STM32\bin`
   - Restart VS Code.

---

### Recommended VS Code Extensions (Windows)

| Extension | Purpose |
|---|---|
| **C/C++** (Microsoft) | IntelliSense, error highlighting |
| **Cortex-Debug** | On-hardware step debugging |

> The **STM32CubeIDE for VS Code** extension from STMicroelectronics is **not required** for a generic Makefile-based workflow. A manual `.vscode/tasks.json` + `.vscode/launch.json` setup is used instead.

---

### Line Ending Setting

Windows and Linux differ in line endings (CRLF vs. LF). Keep all source files as **LF** to prevent Git from detecting false changes:

- In the bottom-right of VS Code, click the line-ending indicator and select **LF**.

---

## 5. VS Code Configuration (Cross-Platform)

Both files live in `.vscode/` and are committed to the repository so both machines share the same configuration.

---

### `.vscode/tasks.json`

Builds the project using `make`. The `"windows"` block overrides the command on Windows only; Ubuntu uses the plain `"command": "make"`.

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Project",
            "type": "shell",
            "command": "make",
            "windows": {
                "command": "C:\\ST\\STM32CubeCLT_1.21.0\\GNU-tools-for-STM32\\bin\\make.exe"
            },
            "args": ["-j4"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        }
    ]
}
```

**Build shortcut:** `Ctrl + Shift + B`

---

### `.vscode/launch.json`

Contains two named debug configurations — one for Ubuntu (using OpenOCD), one for Windows (using ST-LINK GDB Server). Select the correct one from the **Run and Debug** dropdown in VS Code.

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug STM32 (Ubuntu)",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/Nucleo-64-Light.elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "device": "STM32F446RE",
            "interface": "swd",
            "toolchainPrefix": "arm-none-eabi",
            "svdFile": "${workspaceRoot}/STM32F446.svd",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32f4x.cfg"
            ]
        },
        {
            "name": "Debug STM32 (Windows)",
            "cwd": "${workspaceRoot}",
            "executable": "${workspaceRoot}/build/Nucleo-64-Light.elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "stlink",
            "device": "STM32F446RE",
            "interface": "swd",
            "svdFile": "${workspaceRoot}/STM32F446.svd",
            "serverpath": "C:\\ST\\STM32CubeCLT_1.21.0\\STLink-gdb-server\\bin\\ST-LINK_gdbserver.exe",
            "armToolchainPath": "C:\\ST\\STM32CubeCLT_1.21.0\\GNU-tools-for-STM32\\bin",
            "stm32cubeprogrammer": "C:\\ST\\STM32CubeCLT_1.21.0\\STM32CubeProgrammer\\bin"
        }
    ]
}
```

> **Important:** The `"executable"` path must match the exact `.elf` filename produced by your Makefile. Verify the filename in the `build/` folder after a successful compile.

---

### SVD File

The `STM32F446.svd` file enables hardware register inspection during debugging. It must be present in the project root. It can be found in STM32CubeIDE installation directories or downloaded from STMicroelectronics.

---

## 6. Makefile Configuration

Two mandatory adjustments in the generated Makefile are required for this project.

---

### Including LVGL and Driver Source Files

All LVGL subdirectories and the ILI9341 driver path must be added to the Makefile source file list using wildcard commands. This ensures all modules are found by the compiler. Example pattern:

```makefile
# LVGL sources (all subdirectories under Core/lvgl-release-v8.3)
C_SOURCES += $(wildcard Core/lvgl-release-v8.3/src/**/*.c)
C_SOURCES += $(wildcard Core/lvgl-release-v8.3/src/*.c)

# ILI9341 driver sources
C_SOURCES += $(wildcard Drivers/ILI9341/*.c)
```

---

### FPU Setting — Software Float Mode

For compatibility with the FreeRTOS context switch mechanism on the STM32F446, the floating-point ABI **must** be set to software mode:

```makefile
FLOAT-ABI = -mfloat-abi=soft
```

> **Reason:** Using hardware float (`-mfloat-abi=hard` or `softfp`) without correctly saving the FPU registers in the RTOS port leads to context switch corruption and undefined behavior at runtime. `soft` avoids this entirely by not using the hardware FPU in the ABI.

---

## 7. ST-LINK Firmware Upgrade (Windows)

When first connecting the Nucleo board to a Windows machine running recent STM32CubeCLT versions, the following error may appear:

```
ST-LINK firmware upgrade required.
```

### Upgrade via Command Line (STM32CubeCLT)

Since the CLT package does not include the graphical upgrade tool, use the batch script provided in the CLT root:

1. Open a terminal (PowerShell or Command Prompt).
2. Navigate to the CLT root folder:
   ```powershell
   cd C:\ST\STM32CubeCLT_1.21.0
   ```
3. Run the upgrade script with the board connected via USB:
   ```powershell
   .\STLinkUpgrade.bat
   ```
4. In the popup window: click **"Open Update Mode"**, then **"Upgrade"**.
5. Wait for the **"Upgrade successful"** message. Do **not** unplug the board during the process.

### Alternative — CLI Method

```powershell
cd C:\ST\STM32CubeCLT_1.21.0\STM32CubeProgrammer\bin
.\STM32_Programmer_CLI.exe -c port=SWD -fwupgrade
```

### Troubleshooting

| Symptom | Fix |
|---|---|
| "No ST-LINK detected" | Unplug board, wait 5 s, replug, retry |
| Upgrade window won't detect board | Close VS Code — another session may hold the ST-LINK |
| Driver not found | Verify **STSW-LINK009** drivers in Windows Device Manager |

---

## 8. Build & Debug Workflow

### Ubuntu

| Action | Command |
|---|---|
| Clean build output | `make clean` |
| Compile (parallel) | `make -j` |
| Debug | Select **"Debug STM32 (Ubuntu)"** in Run & Debug → press **F5** |

### Windows

| Action | Command / Shortcut |
|---|---|
| Clean build output | `make clean` (in VS Code terminal) |
| Compile | `Ctrl + Shift + B` |
| Debug | Select **"Debug STM32 (Windows)"** in Run & Debug → press **F5** |

---

## 9. Syncing Between Machines (Git Workflow)

### Pushing changes (from either machine)

```bash
git add .
git commit -m "Your descriptive commit message"
git push
```

### Pulling changes (on the other machine)

```bash
git pull
```

The `.vscode/launch.json` and `.vscode/tasks.json` files are shared via Git, so both debug configurations are always available on both machines. Select the appropriate configuration from the **Run and Debug** dropdown.

---

*Generated from a verified, working cross-platform build and debug session on Ubuntu 24.04 and Windows 11.*
