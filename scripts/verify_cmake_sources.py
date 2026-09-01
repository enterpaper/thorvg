#!/usr/bin/env python3
"""Verify that the CMakeLists.txt file lists match the source tree.

For every conditional section in CMakeLists.txt (THORVG_ENABLE_* /
THORVG_USE_EXTERNAL_* options, plus the unconditional base lists) that
contributes to PROJECT_HEADERS / PROJECT_SOURCES, this script checks both
directions against the files that actually exist in the repository:

  1. Files that exist on disk but are NOT listed in CMakeLists.txt
     (a source file was added but the CMake module list was not updated).
  2. Files that ARE listed in CMakeLists.txt but no longer exist on disk
     (a source file was deleted but the CMake module list was not updated).

A file found on disk is attributed to the module that owns the nearest
directory with CMake entries, so both cases are caught:
  - a new file dropped into an already-listed directory,
  - a whole new subdirectory under a module root (e.g. a new engine).

Additional diagnostics (reported as warnings):
  - entries listed more than once,
  - entries placed in the wrong list (.cpp in PROJECT_HEADERS etc.),
  - path spelling that differs from the disk only by case (breaks builds
    on case-sensitive filesystems).

Usage:
    python scripts/verify_cmake_sources.py [options]

Options:
    --root DIR      repository root (default: parent of scripts/)
    --cmake FILE    CMakeLists.txt to verify (default: <root>/CMakeLists.txt)
    --module WORD   only report modules whose name contains WORD (repeatable)
    --no-orphans    do not report files that no module claims
    --ignore PATH   extra list entry to skip, e.g. generated files (repeatable)
    --list-modules  print the discovered modules and exit
    -v, --verbose   also print modules that are in sync

Exit codes:
    0   all lists are in sync
    1   discrepancies found
    2   input or parse error
"""

import argparse
import os
import re
import sys
from collections import OrderedDict

BASE_MODULE = "BASE"
BASE_MODULE_DISPLAY = "BASE (unconditional)"

PROJECT_LIST_VARS = ("PROJECT_HEADERS", "PROJECT_SOURCES")
PATH_PREFIXES = ("${CMAKE_SOURCE_DIR}/", "${CMAKE_CURRENT_SOURCE_DIR}/")

# Extensions treated as source/header files when scanning the source tree.
SOURCE_EXTENSIONS = (".inc.h", ".h", ".hh", ".hpp", ".hxx", ".inl", ".inc",
                     ".cpp", ".cc", ".cxx", ".c", ".mm", ".m")
# Extensions that belong in PROJECT_SOURCES (everything else is a header).
COMPILED_EXTENSIONS = (".cpp", ".cc", ".cxx", ".c", ".mm", ".m")

# Entries that are expected NOT to exist on disk: generated at build time or
# explicit placeholders in CMakeLists.txt. Adjust when new ones appear.
IGNORED_ENTRIES = ()

# Source directories that CMakeLists.txt deliberately does not build
# (meson-only bindings), excluded from the "not claimed by any module" check.
EXCLUDED_DIRS = ("src/bindings",)

# Top-level directories scanned for source files.
SCAN_ROOTS = ("src", "inc")

COMMAND_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*\(")


class CmakeParseError(Exception):
    pass


class Entry(object):
    """One file reference parsed from a PROJECT_HEADERS/PROJECT_SOURCES list."""

    __slots__ = ("path", "key", "list_var", "module", "branch", "line")

    def __init__(self, path, list_var, module, branch, line):
        self.path = path            # repo-relative posix path, as written
        self.key = os.path.normcase(path)
        self.list_var = list_var    # PROJECT_HEADERS or PROJECT_SOURCES
        self.module = module        # outermost option name (or BASE)
        self.branch = branch        # full condition path ("" for base lists)
        self.line = line            # line number of the entry in CMakeLists.txt


def parent_dir(posix_path):
    return posix_path.rsplit("/", 1)[0] if "/" in posix_path else ""


def is_compiled(posix_path):
    return posix_path.endswith(COMPILED_EXTENSIONS)


def is_excluded(posix_path):
    for excluded in EXCLUDED_DIRS:
        if posix_path == excluded or posix_path.startswith(excluded + "/"):
            return True
    return False


def clean_cond(text):
    """Turn '${VAR} OR ${VAR2}' into 'VAR OR VAR2' for display/keys."""
    return text.replace("${", "").replace("}", "").strip()


def cond_text(tokens):
    return " ".join(text for text, _ in tokens)


def module_of(frames):
    if not frames:
        return BASE_MODULE
    return clean_cond(frames[0]["conds"][0])


def branch_label(frames):
    if not frames:
        return ""
    parts = []
    for frame in frames:
        conds = [clean_cond(c) for c in frame["conds"]]
        if frame["negated"]:
            parts.append("else() of " + " / ".join("if(%s)" % c for c in conds))
        else:
            parts.append("if(%s)" % conds[-1])
    return " > ".join(parts)


def iter_commands(lines):
    """Yield (name, tokens, start_line) for every CMake command invocation.

    tokens is a list of (text, line_number) pairs. Handles multi-line
    commands, quoted arguments (parens inside strings are ignored) and
    comments ('#' up to end of line).
    """
    name = None
    depth = 0
    in_string = False
    start_line = -1
    tokens = []

    for lineno, raw in enumerate(lines, 1):
        if name is None:
            stripped = raw.lstrip()
            match = COMMAND_RE.match(stripped)
            if not match:
                continue
            name = match.group(1)
            start_line = lineno
            depth = 1
            in_string = False
            rest = stripped[match.end():]
        else:
            rest = raw

        pending = []
        i = 0
        while i < len(rest):
            ch = rest[i]
            if in_string:
                pending.append(ch)
                if ch == "\\" and i + 1 < len(rest):
                    pending.append(rest[i + 1])
                    i += 1
                elif ch == '"':
                    in_string = False
            elif ch == '"':
                in_string = True
                pending.append(ch)
            elif ch == "#":
                break  # comment runs to the end of the line
            elif ch == "(":
                depth += 1
                pending.append(ch)
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    text = "".join(pending).strip()
                    if text:
                        tokens.append((text, lineno))
                    yield name, tokens, start_line
                    name, depth, in_string, tokens = None, 0, False, []
                    break
                pending.append(ch)
            elif ch.isspace():
                text = "".join(pending)
                if text:
                    tokens.append((text, lineno))
                pending = []
            else:
                pending.append(ch)
            i += 1

        if name is not None:
            text = "".join(pending)
            if text:
                tokens.append((text, lineno))

    if name is not None:
        raise CmakeParseError(
            "unterminated command '%s' starting at line %d" % (name, start_line))


def parse_cmake(path):
    """Parse CMakeLists.txt.

    Returns (entries, warnings, blocks):
      entries   list of Entry, in file order
      warnings  list of (message, line) parse-level warnings
      blocks    {(module, branch, list_var): start line of last list command}
    """
    try:
        with open(path, "rb") as fh:
            data = fh.read()
    except OSError as exc:
        raise CmakeParseError("cannot read %s: %s" % (path, exc))
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        text = data.decode("utf-8", errors="replace")

    entries = []
    warnings = []
    blocks = {}
    frames = []

    def require_frame(keyword, line):
        if not frames:
            raise CmakeParseError(
                "%s() without matching if() at line %d" % (keyword, line))

    for name, tokens, start_line in iter_commands(text.splitlines()):
        if name == "if":
            frames.append({"conds": [cond_text(tokens)], "negated": False})
        elif name == "elseif":
            require_frame("elseif", start_line)
            frames[-1]["conds"].append(cond_text(tokens))
            frames[-1]["negated"] = False
        elif name == "else":
            require_frame("else", start_line)
            frames[-1]["negated"] = True
        elif name == "endif":
            require_frame("endif", start_line)
            frames.pop()
        elif name in ("set", "list"):
            texts = [t for t, _ in tokens]
            if name == "set":
                if not texts:
                    continue
                var, path_tokens = texts[0], tokens[1:]
            else:
                if len(texts) < 2:
                    continue
                if texts[0] != "APPEND":
                    if texts[1] in PROJECT_LIST_VARS:
                        warnings.append((
                            "list(%s ...) on %s is not supported by this check"
                            % (texts[0], texts[1]), start_line))
                    continue
                var, path_tokens = texts[1], tokens[2:]
            if var not in PROJECT_LIST_VARS:
                continue

            module = module_of(frames)
            branch = branch_label(frames)
            blocks[(module, branch, var)] = start_line
            for token, token_line in path_tokens:
                rel = None
                for prefix in PATH_PREFIXES:
                    if token.startswith(prefix):
                        rel = token[len(prefix):].replace("\\", "/")
                        break
                if rel is None:
                    if not token.startswith("${"):
                        warnings.append((
                            "entry '%s' does not use a ${CMAKE_SOURCE_DIR}/ prefix"
                            % token, token_line))
                    continue
                entries.append(Entry(rel, var, module, branch, token_line))

    if frames:
        raise CmakeParseError(
            "unbalanced if()/endif() at end of file (depth %d)" % len(frames))
    return entries, warnings, blocks


def scan_disk(root):
    """Walk SCAN_ROOTS and index repository files.

    Returns (all_by_key, source_files):
      all_by_key     {normcase(key): actual posix path} for every file
                     (used for existence checks of listed entries)
      source_files   posix paths of files with a source/header extension
                     (used to find files missing from the CMake lists)
    """
    all_by_key = {}
    source_files = []
    for scan_root in SCAN_ROOTS:
        top = os.path.join(root, scan_root)
        if not os.path.isdir(top):
            continue
        for dirpath, dirnames, filenames in os.walk(top):
            dirnames.sort()
            filenames.sort()
            for filename in filenames:
                rel = os.path.relpath(
                    os.path.join(dirpath, filename), root).replace(os.sep, "/")
                all_by_key[os.path.normcase(rel)] = rel
                if filename.endswith(SOURCE_EXTENSIONS):
                    source_files.append(rel)
    return all_by_key, source_files


def nearest_listed_dir(posix_dir, dir_entries):
    """Find the closest ancestor directory that has CMake list entries."""
    current = posix_dir
    while current:
        if current in dir_entries:
            return current
        current = parent_dir(current)
    return None


def find_block_line(blocks, module, branch, list_var):
    """Line of the list block to suggest for an addition (best effort)."""
    line = blocks.get((module, branch, list_var))
    if line is not None:
        return line
    for (m, _b, v), ln in blocks.items():
        if m == module and v == list_var:
            return ln
    return None


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Check CMakeLists.txt PROJECT_HEADERS/PROJECT_SOURCES "
                    "against the source tree.")
    parser.add_argument("--root", help="repository root "
                        "(default: parent of the scripts/ folder)")
    parser.add_argument("--cmake", help="CMakeLists.txt to verify "
                        "(default: <root>/CMakeLists.txt)")
    parser.add_argument("--module", action="append", default=[],
                        help="only report modules whose name contains WORD "
                        "(repeatable)")
    parser.add_argument("--no-orphans", dest="orphans", action="store_false",
                        default=True,
                        help="do not report files claimed by no module")
    parser.add_argument("--ignore", action="append", default=[],
                        help="extra list entry to skip, e.g. generated files "
                        "(repeatable)")
    parser.add_argument("--list-modules", action="store_true",
                        help="print the discovered modules and exit")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="also print modules that are in sync")
    args = parser.parse_args(argv)

    default_root = os.path.dirname(
        os.path.dirname(os.path.abspath(__file__)))
    root = os.path.abspath(args.root) if args.root else default_root
    cmake_path = (os.path.abspath(args.cmake) if args.cmake
                  else os.path.join(root, "CMakeLists.txt"))

    try:
        entries, parse_warnings, blocks = parse_cmake(cmake_path)
    except CmakeParseError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2

    if not entries:
        print("error: no PROJECT_HEADERS/PROJECT_SOURCES entries found in %s"
              % cmake_path, file=sys.stderr)
        return 2

    # Index the parsed entries.
    entries_by_key = OrderedDict()
    dir_entries = OrderedDict()
    module_order = []
    module_entry_count = OrderedDict()
    module_dirs = {}
    for entry in entries:
        entries_by_key.setdefault(entry.key, []).append(entry)
        dir_entries.setdefault(parent_dir(entry.path), []).append(entry)
        if entry.module not in module_entry_count:
            module_entry_count[entry.module] = 0
            module_order.append(entry.module)
        module_entry_count[entry.module] += 1
        module_dirs.setdefault(entry.module, set()).add(parent_dir(entry.path))

    if args.list_modules:
        print("modules discovered in %s:" % cmake_path)
        for index, module in enumerate(module_order, 1):
            display = (BASE_MODULE_DISPLAY if module == BASE_MODULE
                       else module)
            print("  %2d. %-58s %3d entries, %d directories"
                  % (index, display, module_entry_count[module],
                     len(module_dirs[module])))
        return 0

    disk_by_key, source_files = scan_disk(root)
    ignored_keys = set(os.path.normcase(p)
                       for p in list(IGNORED_ENTRIES) + list(args.ignore))

    # Direction 2: listed in CMake but missing from disk.
    stale = OrderedDict()          # module -> [Entry]
    ignored_entries = []
    case_mismatches = []           # (Entry, on-disk path)
    for entry in entries:
        if entry.key in ignored_keys:
            ignored_entries.append(entry)
            continue
        actual = disk_by_key.get(entry.key)
        if actual is None:
            stale.setdefault(entry.module, []).append(entry)
        elif actual != entry.path:
            case_mismatches.append((entry, actual))

    # Direction 1: on disk but missing from every CMake list.
    missing = OrderedDict()        # module -> [(path, ref_dir, exact_dir)]
    orphans = []
    listed_keys = set(entries_by_key)
    for rel in source_files:
        key = os.path.normcase(rel)
        if key in listed_keys or key in ignored_keys or is_excluded(rel):
            continue
        ref_dir = nearest_listed_dir(parent_dir(rel), dir_entries)
        if ref_dir is None:
            orphans.append(rel)
            continue
        owner = dir_entries[ref_dir][0].module
        missing.setdefault(owner, []).append(
            (rel, ref_dir, parent_dir(rel) == ref_dir))

    def module_selected(module):
        if not args.module:
            return True
        return any(word.lower() in module.lower() for word in args.module)

    # Warnings.
    warn_items = []
    for lst in entries_by_key.values():
        if len(lst) > 1:
            warn_items.append("duplicate entry %s listed %d times (lines %s)"
                              % (lst[0].path, len(lst),
                                 ", ".join(str(e.line) for e in lst)))
    for entry in entries:
        if entry.list_var == "PROJECT_HEADERS" and is_compiled(entry.path):
            warn_items.append("%s looks misplaced in PROJECT_HEADERS (line %d)"
                              % (entry.path, entry.line))
        elif entry.list_var == "PROJECT_SOURCES" and not is_compiled(entry.path):
            warn_items.append("%s looks misplaced in PROJECT_SOURCES (line %d)"
                              % (entry.path, entry.line))
    for message, line in parse_warnings:
        warn_items.append("%s (line %d)" % (message, line)
                          if line else message)

    # Report.
    out = []
    out.append("CMake source-list verification")
    out.append("  root:    %s" % root.replace(os.sep, "/"))
    out.append("  cmake:   %s" % cmake_path.replace(os.sep, "/"))
    out.append("  entries: %d file entries in %d module section(s)"
               % (len(entries), len(module_order)))
    out.append("")

    error_count = 0
    for module in module_order:
        if not module_selected(module):
            continue
        mod_missing = missing.get(module, [])
        mod_stale = stale.get(module, [])
        mod_case = [pair for pair in case_mismatches if pair[0].module == module]
        display = BASE_MODULE_DISPLAY if module == BASE_MODULE else module

        if not (mod_missing or mod_stale or mod_case):
            if args.verbose:
                out.append("=== %s === OK (%d entries listed)"
                           % (display, module_entry_count[module]))
                out.append("")
            continue

        out.append("=== %s ===" % display)

        if mod_missing:
            out.append("")
            out.append("  [MISSING IN CMAKE] present in the source tree "
                       "but not listed:")
            groups = OrderedDict()
            for rel, ref_dir, exact in mod_missing:
                own_dir = parent_dir(rel)
                group = groups.setdefault(
                    own_dir, {"ref": ref_dir, "exact": exact, "files": []})
                group["files"].append(rel)
            for own_dir, group in groups.items():
                if not group["exact"]:
                    out.append('    (new subdirectory "%s/" - no list '
                               "references it;" % own_dir)
                    out.append("     nearest listed directory: %s)"
                               % group["ref"])
                for rel in group["files"]:
                    out.append("    - %s" % rel)
                branch = dir_entries[group["ref"]][0].branch
                vars_needed = []
                if any(not is_compiled(f) for f in group["files"]):
                    vars_needed.append("PROJECT_HEADERS")
                if any(is_compiled(f) for f in group["files"]):
                    vars_needed.append("PROJECT_SOURCES")
                block_info = []
                for var in vars_needed:
                    line_no = find_block_line(blocks, module, branch, var)
                    if line_no is not None:
                        block_info.append("%s at %s:%d"
                                          % (var, os.path.basename(cmake_path),
                                             line_no))
                where = ('under "%s"' % branch) if branch \
                    else "in the top-level list"
                suffix = "" if group["exact"] \
                    else " (or set up a new option section for that subdirectory)"
                out.append("      hint: add to %s %s%s%s"
                           % (" / ".join(vars_needed), where,
                              " [%s]" % ", ".join(block_info)
                              if block_info else "",
                              suffix))
            error_count += len(mod_missing)

        if mod_stale:
            out.append("")
            out.append("  [STALE ENTRY] listed in CMakeLists.txt but "
                       "missing from the source tree:")
            for entry in mod_stale:
                out.append("    - %s  (%s, line %d, branch: %s)"
                           % (entry.path, entry.list_var, entry.line,
                              entry.branch or "unconditional"))
            out.append("      hint: remove the entries, those files no "
                       "longer exist")
            error_count += len(mod_stale)

        if mod_case:
            out.append("")
            out.append("  [CASE MISMATCH] listed spelling differs from "
                       "the file on disk:")
            for entry, actual in mod_case:
                out.append('    - listed "%s" vs on-disk "%s" (%s, line %d)'
                           % (entry.path, actual, entry.list_var, entry.line))
            error_count += len(mod_case)

        out.append("")

    if args.orphans and orphans and not args.module:
        out.append("=== files not claimed by any CMake module ===")
        out.append("")
        out.append("  [NOT COVERED] no PROJECT_HEADERS/PROJECT_SOURCES "
                   "entry can own these:")
        for rel in orphans:
            out.append("    - %s" % rel)
        out.append("      hint: add a module section for them, or extend "
                   "EXCLUDED_DIRS in this script")
        out.append("")
        error_count += len(orphans)

    if ignored_entries:
        out.append("note: %d known generated/placeholder entries skipped:"
                   % len(ignored_entries))
        for entry in ignored_entries:
            out.append("    - %s (line %d)" % (entry.path, entry.line))
        out.append("")

    if warn_items:
        out.append("warnings (%d):" % len(warn_items))
        for item in warn_items:
            out.append("  - %s" % item)
        out.append("")

    out.append("-" * 64)
    if error_count == 0 and not warn_items:
        out.append("Result: OK - %d entries in %d modules all match the "
                   "source tree." % (len(entries), len(module_order)))
    elif error_count == 0:
        out.append("Result: OK with warnings - %d entries in %d modules, "
                   "%d warning(s)."
                   % (len(entries), len(module_order), len(warn_items)))
    else:
        out.append("Result: FAILED - %d error(s), %d warning(s)."
                   % (error_count, len(warn_items)))
    print("\n".join(out))

    return 1 if error_count else 0


if __name__ == "__main__":
    sys.exit(main())
