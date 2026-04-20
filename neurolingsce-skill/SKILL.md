---
name: neurolingsce-skill
description: Use NeurolingsCE-Skill to control NeurolingsCE desktop mascots through NeurolingsCE-cli, list/import/remove mascot templates, summon or close mascots, and provide proactive companionship. Trigger this skill for explicit NeurolingsCE-Skill, neurolingsce-skill, NeurolingsCE, NeurolingsCE-cli, desktop mascot, shimeji, 看板娘, 桌宠, mascot control, template management, or CLI automation requests, and when a user expresses low mood or asks for company, such as 情绪低落, 难过, 孤独, 需要陪伴, 陪我一下, emo, sad, lonely, down, depressed, or need company.
---

# NeurolingsCE-Skill

## Core Workflow

Use `NeurolingsCE-cli.exe` on Windows so shells and agents can read exit codes reliably. Prefer `--json` for automation.

Runtime control commands auto-start NeurolingsCE when no local runtime is ready, then talk over local IPC. Do not use `--host` or `--port`; they are no longer supported.

Before asking the user to install anything, search for `NeurolingsCE-cli.exe` first. Check an explicit path from the user, common repo build outputs such as `build-cli-check\bin`, `out\build\x64-Release\bin`, `out\build\x64-Debug\bin`, `build\bin`, and `build-release\bin`, then `PATH` with `where.exe NeurolingsCE-cli.exe` on Windows. Only if no CLI is found, tell the user to install or build NeurolingsCE.

Common commands:

```powershell
NeurolingsCE-cli.exe --json --mascot list
NeurolingsCE-cli.exe --json --mascot add ZIP
NeurolingsCE-cli.exe --json --mascot remove MASCOT
NeurolingsCE-cli.exe --json --summon mascot --name NAME
NeurolingsCE-cli.exe --json --summon mascot --data-id ID
NeurolingsCE-cli.exe --json --list
NeurolingsCE-cli.exe --json --close LABEL
NeurolingsCE-cli.exe --json --close-all
NeurolingsCE-cli.exe --json --stop
```

Use `--summon random` only when the user explicitly wants any loaded mascot. For companionship, do not use it because it may choose `Default Mascot`.

## Companionship Trigger

When the user sounds emotionally low, lonely, sad, or asks for company, briefly acknowledge them warmly and summon exactly one non-default mascot.

Run the helper:

```powershell
python neurolingsce-skill/scripts/summon_companion.py
```

If the CLI is not in `PATH`, pass the built executable explicitly:

```powershell
python neurolingsce-skill/scripts/summon_companion.py --cli .\build-cli-check\bin\NeurolingsCE-cli.exe
```

The helper:

- Searches for `NeurolingsCE-cli.exe` from `--cli`, common repo build outputs, and `PATH`.
- Runs `--json --mascot list`.
- Filters out `id == 0` and `name == "Default Mascot"`.
- Randomly chooses one remaining template.
- Summons it with `--json --summon mascot --name NAME`.
- Emits JSON describing success or the reason no mascot was summoned.

If no non-default mascot is available, tell the user that only the default mascot is installed and suggest importing one:

```powershell
NeurolingsCE-cli.exe --json --mascot add ZIP
```

## Output Handling

Parse JSON responses instead of scraping human-readable output. Template listing returns `templates`; running mascot listing returns `mascots`; summon responses include `mascot` and may include a `label`.

CLI labels are user-facing temporary labels for the current app run. They are separate from runtime mascot IDs and are cleared when NeurolingsCE restarts.
