#!/usr/bin/env python3
"""Summon one random non-default NeurolingsCE mascot."""

from __future__ import annotations

import argparse
import json
import random
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


DEFAULT_NAME = "Default Mascot"


def write_json(payload: dict[str, Any], exit_code: int) -> int:
    print(json.dumps(payload, ensure_ascii=False, separators=(",", ":")))
    return exit_code


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def skill_root() -> Path:
    return Path(__file__).resolve().parents[1]


def default_record_path() -> Path:
    return skill_root() / "cache" / "neurolingsce-cli-path.json"


def recorded_cli_path(record_path: Path) -> Path | None:
    if not record_path.is_file():
        return None
    try:
        payload = json.loads(record_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None
    if not isinstance(payload, dict):
        return None
    cli = payload.get("cli")
    if not isinstance(cli, str) or not cli:
        return None
    path = Path(cli)
    return path if path.is_file() else None


def candidate_cli_paths(explicit: str | None) -> list[Path]:
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit))

    root = repo_root()
    candidates.extend(
        [
            root / "build-cli-check" / "bin" / "NeurolingsCE-cli.exe",
            root / "out" / "build" / "x64-Release" / "bin" / "NeurolingsCE-cli.exe",
            root / "out" / "build" / "x64-Debug" / "bin" / "NeurolingsCE-cli.exe",
            root / "build" / "bin" / "NeurolingsCE-cli.exe",
            root / "build-release" / "bin" / "NeurolingsCE-cli.exe",
            root / "publish" / "Windows" / "release" / "NeurolingsCE-cli.exe",
            root / "publish" / "Windows" / "debug" / "NeurolingsCE-cli.exe",
        ]
    )

    for name in ("NeurolingsCE-cli.exe", "NeurolingsCE-cli"):
        found = shutil.which(name)
        if found:
            candidates.append(Path(found))
    return candidates


def find_cli(explicit: str | None, record_path: Path) -> Path | None:
    recorded = recorded_cli_path(record_path)
    if recorded is not None:
        return recorded
    for candidate in candidate_cli_paths(explicit):
        if candidate.is_file():
            return candidate
    return None


def run_cli(cli: Path, args: list[str]) -> tuple[int, str, str]:
    completed = subprocess.run(
        [str(cli), *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    return completed.returncode, completed.stdout.strip(), completed.stderr.strip()


def parse_cli_json(stdout: str, command: str) -> tuple[dict[str, Any] | None, dict[str, Any] | None]:
    try:
        value = json.loads(stdout)
    except json.JSONDecodeError as exc:
        return None, {
            "ok": False,
            "code": "invalid_cli_json",
            "message": f"{command} did not return valid JSON.",
            "details": str(exc),
            "stdout": stdout,
        }
    if not isinstance(value, dict):
        return None, {
            "ok": False,
            "code": "invalid_cli_json",
            "message": f"{command} returned JSON that was not an object.",
            "stdout": stdout,
        }
    return value, None


def template_name(template: dict[str, Any]) -> str:
    value = template.get("name", "")
    return value if isinstance(value, str) else ""


def is_non_default_template(template: dict[str, Any]) -> bool:
    name = template_name(template).strip()
    if not name:
        return False
    if name.casefold() == DEFAULT_NAME.casefold():
        return False
    template_id = template.get("id")
    if template_id == 0 or template_id == "0":
        return False
    return True


def loaded_templates(payload: dict[str, Any]) -> list[dict[str, Any]]:
    value = payload.get("templates", payload.get("loaded_mascots", []))
    if not isinstance(value, list):
        return []
    return [item for item in value if isinstance(item, dict)]


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Summon one random non-default NeurolingsCE mascot."
    )
    parser.add_argument("--cli", help="Path to NeurolingsCE-cli.exe.")
    parser.add_argument(
        "--record",
        default=str(default_record_path()),
        help="JSON record produced by find_neurolingsce_cli.py.",
    )
    parser.add_argument(
        "--label",
        help="Optional user-facing CLI label to pass to --summon.",
    )
    args = parser.parse_args(argv)

    record_path = Path(args.record).expanduser()
    cli = find_cli(args.cli, record_path)
    if cli is None:
        searched = [str(path) for path in candidate_cli_paths(args.cli)]
        return write_json(
            {
                "ok": False,
                "code": "cli_not_found",
                "message": "Could not find NeurolingsCE-cli.exe after checking the recorded path, explicit path, repo build locations, and PATH. Run find_neurolingsce_cli.py, install/build NeurolingsCE, or pass --cli PATH.",
                "record": str(record_path),
                "searched": searched,
            },
            127,
        )

    code, stdout, stderr = run_cli(cli, ["--json", "--mascot", "list"])
    if code != 0:
        payload, parse_error = parse_cli_json(stdout, "--mascot list")
        return write_json(
            {
                "ok": False,
                "code": "template_list_failed",
                "message": "Could not list mascot templates.",
                "exit_code": code,
                "cli": str(cli),
                "response": payload,
                "parse_error": parse_error,
                "stderr": stderr,
            },
            code,
        )

    payload, parse_error = parse_cli_json(stdout, "--mascot list")
    if parse_error is not None or payload is None:
        return write_json(parse_error or {}, 1)

    choices = [template for template in loaded_templates(payload) if is_non_default_template(template)]
    if not choices:
        return write_json(
            {
                "ok": False,
                "code": "no_non_default_mascots",
                "message": "No non-default mascot template is installed. Nothing was summoned.",
                "cli": str(cli),
            },
            2,
        )

    chosen = random.choice(choices)
    name = template_name(chosen)
    summon_args = ["--json", "--summon", "mascot", "--name", name]
    if args.label:
        summon_args.append(args.label)

    code, stdout, stderr = run_cli(cli, summon_args)
    summon_payload, parse_error = parse_cli_json(stdout, "--summon mascot")
    if code != 0:
        return write_json(
            {
                "ok": False,
                "code": "summon_failed",
                "message": f"Could not summon mascot {name}.",
                "exit_code": code,
                "cli": str(cli),
                "chosen": chosen,
                "response": summon_payload,
                "parse_error": parse_error,
                "stderr": stderr,
            },
            code,
        )
    if parse_error is not None or summon_payload is None:
        return write_json(parse_error or {}, 1)

    return write_json(
        {
            "ok": True,
            "cli": str(cli),
            "chosen": chosen,
            "summon": summon_payload,
        },
        0,
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
