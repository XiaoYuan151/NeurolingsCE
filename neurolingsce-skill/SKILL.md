---
name: neurolingsce-skill
description: Use NeurolingsCE-Skill only to control or summon existing NeurolingsCE desktop mascot templates through NeurolingsCE-cli; never create, draw, generate, import, or modify mascot resources. Trigger this skill for explicit NeurolingsCE-Skill, neurolingsce-skill, NeurolingsCE, NeurolingsCE-cli, desktop mascot, shimeji, 看板娘, 桌宠, mascot control, template listing, close/summon requests, CLI automation requests, or Chinese requests like 帮我生成一只xxx桌宠, where 生成 means summon an existing mascot with NeurolingsCE-cli, not create mascot art/assets. Also trigger when a user expresses low mood or asks for company, such as 情绪低落, 难过, 孤独, 需要陪伴, 陪我一下, emo, sad, lonely, down, depressed, or need company.
---

# NeurolingsCE-Skill

## Core Workflow

Use `NeurolingsCE-cli.exe` on Windows so shells and agents can read exit codes reliably. Prefer `--json` for automation.

Runtime control commands auto-start NeurolingsCE when no local runtime is ready, then talk over local IPC. Do not use `--host` or `--port`; they are no longer supported.

Before asking the user to install anything, search for `NeurolingsCE-cli.exe` first. Use the finder helper to check an explicit path, common repo build outputs, `PATH`, and common install roots, then record the discovered path for later calls:

```powershell
python neurolingsce-skill/scripts/find_neurolingsce_cli.py
```

Pass extra hints when needed:

```powershell
python neurolingsce-skill/scripts/find_neurolingsce_cli.py --cli C:\path\to\NeurolingsCE-cli.exe
python neurolingsce-skill/scripts/find_neurolingsce_cli.py --search-root D:\Apps
```

The finder writes `neurolingsce-skill/cache/neurolingsce-cli-path.json`. If no CLI is found after this search, tell the user to install or build NeurolingsCE. Do not create mascot resources.

Common commands:

```powershell
NeurolingsCE-cli.exe --json --mascot list
NeurolingsCE-cli.exe --json --summon mascot --name NAME
NeurolingsCE-cli.exe --json --summon mascot --data-id ID
NeurolingsCE-cli.exe --json --list
NeurolingsCE-cli.exe --json --close LABEL
NeurolingsCE-cli.exe --json --close-all
NeurolingsCE-cli.exe --json --stop
```

Use `--summon random` only when the user explicitly wants any loaded mascot. For companionship, do not use it because it may choose `Default Mascot`.

## Request Semantics

Interpret requests like `帮我生成一只xxx桌宠`, `生成一个xxx桌宠`, or `来一只xxx桌宠` as requests to summon an existing mascot template with `NeurolingsCE-cli.exe --json --summon mascot --name NAME` or `--data-id ID`.

Do not treat these phrases as requests to create new mascot resources, draw character art, generate sprites, build a Shimeji pack, or import assets unless the user explicitly asks for new resources, images, sprites, XML, ZIP packs, or asset creation.

If `xxx` names a mascot, first list templates with `NeurolingsCE-cli.exe --json --mascot list`, match the requested name against available templates, then summon the matched template. If no match exists, tell the user exactly that this template is not installed or not found. Do not create assets, generate images, generate sprites, build XML, build ZIP packs, import a replacement, or choose a different mascot unless the user asks for a different installed template.

This skill does not generate anything. Treat "生成桌宠" as wording for "summon an installed mascot"; if the installed template does not exist, stop after telling the user it does not exist.

## Companionship Trigger

When the user sounds emotionally low, lonely, sad, or asks for company, briefly acknowledge them warmly and summon exactly one non-default mascot.

Keep the skill invocation invisible. Do not tell the user that you are using a skill, running a helper, checking a cache, or calling NeurolingsCE-cli unless the call fails and the failure matters to the user.

By default, only call `NeurolingsCE-cli.exe`. Do not start `NeurolingsCE.exe`, do not open the GUI, and do not start runtime mode unless the user explicitly asks for that behavior.

Run the helper:

```powershell
python neurolingsce-skill/scripts/summon_companion.py
```

If the CLI is not in `PATH`, pass the built executable explicitly:

```powershell
python neurolingsce-skill/scripts/summon_companion.py --cli .\build-cli-check\bin\NeurolingsCE-cli.exe
```

The helper:

- Reads `cache/neurolingsce-cli-path.json` when available, then searches for `NeurolingsCE-cli.exe` from `--cli`, common repo build outputs, and `PATH`.
- Runs `--json --mascot list`.
- Filters out `id == 0` and `name == "Default Mascot"`.
- Randomly chooses one remaining template.
- Summons it with `--json --summon mascot --name NAME`.
- Emits JSON describing success or the reason no mascot was summoned.

If no non-default mascot is available, tell the user that no non-default mascot template is installed. Do not create, download, import, or suggest a substitute mascot.

## Output Handling

Parse JSON responses instead of scraping human-readable output. Template listing returns `templates`; running mascot listing returns `mascots`; summon responses include `mascot` and may include a `label`.

CLI labels are user-facing temporary labels for the current app run. They are separate from runtime mascot IDs and are cleared when NeurolingsCE restarts.
