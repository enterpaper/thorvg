"""Compile the bgfx engine .sc files into bgfx embedded shader headers.

Based on the canvaskit-wasm vg-renderer shader pipeline: every .sc source is
compiled once per backend with bgfx shaderc (--bin2c) and the per-backend
chunks are concatenated into <stem>.bin.h next to the sources. Those headers
are consumed by tvgBgfxContext.cpp through BGFX_EMBEDDED_SHADER().

Usage:
    python compile_shaders.py [--shaderc PATH] [--verbose] [--clean]
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Sequence


SCRIPT_DIR = Path(__file__).resolve().parent


@dataclass(frozen=True)
class Backend:
    platform: str
    profile: str
    suffix: str
    optimize: str | None = None


BACKENDS = (
    # bgfx 新版 shaderc 的 profile 表（shaderc.cpp s_profiles）：
    # 桌面 GLSL 最低 330，ESSL 最低 300_es —— 旧的 120 / 100_es 已被移除
    Backend("linux", "330", "glsl"),
    Backend("android", "300_es", "essl"),
    Backend("linux", "spirv", "spv"),
    Backend("linux", "wgsl", "wgsl"),
    Backend("windows", "s_5_0", "dxbc", "3"),
    Backend("windows", "s_6_0", "dxil", "3"),
    Backend("ios", "metal", "mtl", "3"),
)


class ShaderCompileError(RuntimeError):
    """Raised when shaderc cannot produce a valid embedded shader output."""


def find_bgfx_root(start: Path, explicit: Path | None = None) -> Path:
    """Locate a bgfx checkout with src/ (explicit path or upwards walk)."""

    if explicit is not None:
        root = explicit.expanduser().resolve()
        if not (root / "src").is_dir():
            raise FileNotFoundError(f"no bgfx src/ directory under {root}")
        return root

    # note: a junction/symlinked thorvg checkout resolves to its real path,
    # so the walk covers the script location and the working directory
    for origin in (start, Path.cwd()):
        for candidate in (origin, *origin.parents):
            for sub in (candidate / "bgfx", candidate / "third_party" / "bgfx"):
                if (sub / "src").is_dir():
                    return sub.resolve()
    raise FileNotFoundError(
        "Unable to locate the bgfx checkout (bgfx/src or third_party/bgfx/src); "
        "pass --bgfx-root PATH"
    )


def find_shaderc(
    bgfx_root: Path,
    explicit: Path | None,
    environment: Mapping[str, str] | None = None,
) -> Path:
    """Find shaderc, preferring the project's MSVC Release build."""

    if explicit is not None:
        candidate = explicit.expanduser().resolve()
        if not candidate.is_file():
            raise FileNotFoundError(f"shaderc was not found at {candidate}")
        return candidate

    candidates = (
        bgfx_root / ".build" / "win64_vs2022" / "bin" / "shadercRelease.exe",
        bgfx_root / ".build" / "win64_vs2022" / "bin" / "shadercDebug.exe",
        bgfx_root / "tools" / "bin" / "windows" / "shaderc.exe",
        bgfx_root / "tools" / "bin" / "windows" / "shaderc",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()

    path_value = (environment or os.environ).get("PATH")
    for command_name in ("shaderc", "shaderc.exe"):
        discovered = shutil.which(command_name, path=path_value)
        if discovered:
            return Path(discovered).resolve()

    raise FileNotFoundError(
        "Unable to find shaderc. Build bgfx's host shaderc or pass --shaderc PATH."
    )


def validate_shaderc_runtime(shaderc: Path) -> None:
    """Validate the DLL layout required by the Windows shaderc binary."""

    if os.name == "nt" and shaderc.suffix.lower() == ".exe":
        dxc = shaderc.parent / "dxcompiler.dll"
        if not dxc.is_file():
            raise FileNotFoundError(
                f"{shaderc.name} requires {dxc.name} beside the executable: {dxc}"
            )


def build_shaderc_command(
    shaderc: Path,
    include_dir: Path,
    source: Path,
    output: Path,
    backend: Backend,
) -> list[str]:
    """Build one shaderc command for a single backend variant."""

    command = [
        str(shaderc),
        "-i",
        str(include_dir),
        "--type",
        "vertex" if source.name.startswith("vs_") else "fragment",
        "--platform",
        backend.platform,
        "-p",
        backend.profile,
        "-f",
        str(source),
        "-o",
        str(output),
        "--bin2c",
        f"{source.stem}_{backend.suffix}",
    ]
    if backend.optimize is not None:
        command.extend(["-O", backend.optimize])
    return command


def _run_shaderc(command: Sequence[str], shader_dir: Path, verbose: bool) -> None:
    if verbose:
        print("$ " + subprocess.list2cmdline([str(part) for part in command]))

    result = subprocess.run(
        list(command),
        cwd=shader_dir,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.returncode != 0:
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        raise ShaderCompileError(
            f"shaderc failed with exit code {result.returncode}: {command[-1]}"
        )
    if result.stderr and verbose:
        print(result.stderr, end="", file=sys.stderr)


def compile_shader(
    shaderc: Path,
    include_dir: Path,
    source: Path,
    verbose: bool = False,
) -> Path:
    """Compile one .sc source and atomically replace its .bin.h output."""

    output = source.with_suffix(".bin.h")
    source_type = "vertex" if source.name.startswith("vs_") else "fragment"
    if not source.name.startswith(("vs_", "fs_")):
        raise ShaderCompileError(f"Unsupported shader source name: {source.name}")

    chunks: list[str] = []
    with tempfile.TemporaryDirectory(prefix=".shaderc-", dir=source.parent) as temp_dir:
        temp_root = Path(temp_dir)
        for backend in BACKENDS:
            temp_output = temp_root / f"{source.stem}_{backend.suffix}.h"
            command = build_shaderc_command(
                shaderc, include_dir, source, temp_output, backend
            )
            command[command.index("--type") + 1] = source_type
            _run_shaderc(command, source.parent, verbose)
            if not temp_output.is_file():
                raise ShaderCompileError(
                    f"shaderc completed without creating {temp_output}"
                )
            chunks.append(temp_output.read_text(encoding="utf-8"))

    chunks.extend(
        [
            f"extern const uint8_t* {source.stem}_pssl;\n",
            f"extern const uint32_t {source.stem}_pssl_size;\n",
        ]
    )
    content = "".join(chunks)

    output_tmp = output.with_name(f".{output.name}.tmp")
    output_tmp.write_text(content, encoding="utf-8", newline="\n")
    os.replace(output_tmp, output)
    return output


def compile_all(
    shader_dir: Path,
    shaderc: Path,
    include_dir: Path,
    clean: bool = False,
    verbose: bool = False,
) -> list[Path]:
    sources = sorted(shader_dir.glob("vs_*.sc")) + sorted(shader_dir.glob("fs_*.sc"))
    if not sources:
        raise ShaderCompileError(f"No vertex or fragment shaders found in {shader_dir}")

    if clean:
        for source in sources:
            source.with_suffix(".bin.h").unlink(missing_ok=True)

    outputs = []
    for source in sources:
        print(f"[{source.name}]")
        outputs.append(compile_shader(shaderc, include_dir, source, verbose))
    return outputs


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compile the ThorVG bgfx engine shaders into embedded .bin.h files."
    )
    parser.add_argument(
        "--bgfx-root",
        type=Path,
        default=None,
        help="Path to the bgfx checkout; defaults to an upwards search from the script.",
    )
    parser.add_argument(
        "--shaderc",
        type=Path,
        help="Path to shaderc; defaults to the bgfx MSVC shadercRelease.exe.",
    )
    parser.add_argument(
        "--shader-dir",
        type=Path,
        default=SCRIPT_DIR,
        help="Directory containing the .sc files.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print every shaderc command and its output.",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove existing .bin.h files before compiling.",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    shader_dir = args.shader_dir.expanduser().resolve()
    bgfx_root = find_bgfx_root(SCRIPT_DIR, args.bgfx_root)
    include_dir = bgfx_root / "src"
    shaderc = find_shaderc(bgfx_root, args.shaderc, os.environ)
    validate_shaderc_runtime(shaderc)

    if args.verbose:
        print(f"shaderc: {shaderc}")
        print(f"shader directory: {shader_dir}")

    outputs = compile_all(
        shader_dir,
        shaderc,
        include_dir,
        clean=args.clean,
        verbose=args.verbose,
    )
    print(f"Generated {len(outputs)} shader headers.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, ShaderCompileError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
