#!/usr/bin/env python3
"""Checks the issue forms, their config, the label list and the triage map before GitHub sees them.

GitHub rejects a whole issue form over one bad character (a tab in the indentation, a duplicate id, an unquoted
`Yes` that YAML reads as true, an option `Area: text` that YAML reads as a mapping) and only says so on the
New issue page. This catches those, and keeps .github/issue-triage.json and .github/labels.json in step with the
forms, so issue-triage.yml never meets an answer it can't label.

Run from anywhere (needs PyYAML):

    python .github/scripts/validate_github_forms.py

Exits 1 when it finds a problem. Under GitHub Actions it also prints annotations that show on the pull request.
"""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path

import yaml

GITHUB = Path(__file__).resolve().parent.parent
ROOT = GITHUB.parent
TEMPLATES = GITHUB / "ISSUE_TEMPLATE"
WORKFLOWS = GITHUB / "workflows"

TOP_LEVEL_KEYS = {"name", "description", "title", "labels", "assignees", "projects", "type", "body"}
ELEMENT_KEYS = {"type", "id", "attributes", "validations"}
ATTRIBUTE_KEYS = {
    "markdown": {"value"},
    "input": {"label", "description", "placeholder", "value"},
    "textarea": {"label", "description", "placeholder", "value", "render"},
    "dropdown": {"label", "description", "multiple", "options", "default"},
    "checkboxes": {"label", "description", "options"},
}
ID_PATTERN = re.compile(r"^[A-Za-z0-9_-]+$")
COLOR_PATTERN = re.compile(r"^[0-9A-Fa-f]{6}$")
PROJECT_KEY_PATTERN = re.compile(r"^[A-Za-z0-9_.-]+/\d+$")
PROJECT_URL_PATTERN = re.compile(r"^https://github\.com/(?:orgs|users)/([^/]+)/projects/(\d+)/?$")
CHECK_RULES = {"nonempty", "pattern", "attachment"}

problems: list[tuple[Path, int | None, str]] = []


def problem(path: Path, message: str, line: int | None = None) -> None:
    problems.append((path, line, message))


def relative(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def check_whitespace(path: Path, text: str) -> None:
    """GitHub's YAML parser refuses tabs where it expects indentation, including inside block scalars (`|`)."""
    for number, line in enumerate(text.splitlines(), start=1):
        column = line.find("\t")
        if column >= 0:
            problem(path, f"tab character at column {column + 1}: indent with spaces only", number)


def load_yaml(path: Path) -> tuple[object, yaml.Node | None]:
    text = path.read_text(encoding="utf-8")
    check_whitespace(path, text)
    try:
        return yaml.safe_load(text), yaml.compose(text)
    except yaml.YAMLError as error:
        mark = getattr(error, "problem_mark", None)
        problem(path, f"YAML syntax error: {getattr(error, 'problem', error)}", mark.line + 1 if mark else None)
        return None, None


def body_lines(root: yaml.Node | None) -> list[int | None]:
    """The line each `body` element starts on, for annotations."""
    if not isinstance(root, yaml.MappingNode):
        return []
    for key, value in root.value:
        if key.value == "body" and isinstance(value, yaml.SequenceNode):
            return [item.start_mark.line + 1 for item in value.value]
    return []


def is_text(value: object) -> bool:
    return isinstance(value, str) and value.strip() != ""


def as_list(value: object) -> list[str]:
    """`labels` / `assignees` may be a list or a comma-separated string."""
    if isinstance(value, str):
        return [part.strip() for part in value.split(",") if part.strip()]
    if isinstance(value, list):
        return value
    return []


def check_labels_file() -> dict[str, str]:
    """Returns the labels in labels.json, lower-case name -> name."""
    path = GITHUB / "labels.json"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        problem(path, f"can't read: {error}")
        return {}

    names: dict[str, str] = {}
    for index, label in enumerate(data.get("labels", [])):
        name = label.get("name")
        where = f"labels[{index}]"
        if not is_text(name) or len(name) > 50:
            problem(path, f"{where}: name must be 1 to 50 characters")
            continue
        if name.lower() in names:
            problem(path, f'{where}: "{name}" is listed twice (GitHub label names ignore case)')
        names[name.lower()] = name
        if not isinstance(label.get("color"), str) or not COLOR_PATTERN.match(label["color"]):
            problem(path, f'{where} "{name}": color must be six hex digits without #')
        description = label.get("description", "")
        if not isinstance(description, str) or len(description) > 100:
            problem(path, f'{where} "{name}": description must be text of at most 100 characters')
        extra = set(label) - {"name", "color", "description"}
        if extra:
            problem(path, f'{where} "{name}": unknown keys {sorted(extra)}')
    return names


def check_element(path: Path, line: int | None, index: int, element: object, ids: set[str], labels: set[str]) -> None:
    where = f"body[{index}]"
    if not isinstance(element, dict):
        problem(path, f"{where}: must be a mapping with type and attributes", line)
        return

    kind = element.get("type")
    if kind not in ATTRIBUTE_KEYS:
        problem(path, f"{where}: type must be one of {sorted(ATTRIBUTE_KEYS)}, not {kind!r}", line)
        return
    for key in set(element) - ELEMENT_KEYS:
        problem(path, f"{where} ({kind}): unknown key {key!r}", line)

    attributes = element.get("attributes")
    if not isinstance(attributes, dict):
        problem(path, f"{where} ({kind}): attributes must be a mapping", line)
        return
    for key in set(attributes) - ATTRIBUTE_KEYS[kind]:
        problem(path, f"{where} ({kind}): unknown attribute {key!r}", line)
    for key in ("label", "description", "placeholder", "value", "render"):
        if key in attributes and not isinstance(attributes[key], str):
            problem(path, f"{where} ({kind}): {key} must be text; quote it (YAML read {attributes[key]!r})", line)

    element_id = element.get("id")
    if element_id is not None:
        if not isinstance(element_id, str) or not ID_PATTERN.match(element_id):
            problem(path, f"{where}: id {element_id!r} may only use letters, digits, - and _", line)
        elif element_id in ids:
            problem(path, f"{where}: id {element_id!r} is used twice", line)
        else:
            ids.add(element_id)

    validations = element.get("validations")
    if validations is not None:
        if kind in ("markdown", "checkboxes"):
            problem(path, f"{where} ({kind}): validations aren't allowed here (checkboxes take required per option)", line)
        elif not isinstance(validations, dict) or set(validations) - {"required"} or not isinstance(validations.get("required", False), bool):
            problem(path, f"{where} ({kind}): validations may only hold required: true / false", line)

    if kind == "markdown":
        if not is_text(attributes.get("value")):
            problem(path, f"{where} (markdown): value must be non-empty text", line)
        return

    label = attributes.get("label")
    if not is_text(label):
        problem(path, f"{where} ({kind}): label must be non-empty text", line)
    elif label.strip().lower() in labels:
        problem(path, f"{where} ({kind}): label {label!r} is used twice", line)
    else:
        labels.add(label.strip().lower())
    if element_id is None:
        problem(path, f"{where} ({kind}): give it an id (needed to pre-fill the field from a URL)", line)

    if kind == "dropdown":
        options = attributes.get("options")
        if not isinstance(options, list) or not options:
            problem(path, f"{where} (dropdown): options must be a non-empty list", line)
            return
        seen: set[str] = set()
        for option in options:
            if not is_text(option):
                problem(path, f"{where} (dropdown): option {option!r} isn't text; quote it", line)
                continue
            if option in seen:
                problem(path, f"{where} (dropdown): option {option!r} is listed twice", line)
            seen.add(option)
            if option.strip().lower() == "none":
                problem(path, f'{where} (dropdown): "None" is reserved by GitHub', line)
            if attributes.get("multiple") and ", " in option:
                problem(path, f"{where} (dropdown): option {option!r} contains ', ', which separates the answers of a multiple-choice dropdown", line)
        if "multiple" in attributes and not isinstance(attributes["multiple"], bool):
            problem(path, f"{where} (dropdown): multiple must be true or false", line)
        default = attributes.get("default")
        if default is not None and (not isinstance(default, int) or isinstance(default, bool) or not 0 <= default < len(options)):
            problem(path, f"{where} (dropdown): default must be an option index from 0 to {len(options) - 1}", line)

    if kind == "checkboxes":
        options = attributes.get("options")
        if not isinstance(options, list) or not options:
            problem(path, f"{where} (checkboxes): options must be a non-empty list", line)
            return
        for option in options:
            if not isinstance(option, dict) or not is_text(option.get("label")):
                problem(path, f"{where} (checkboxes): each option needs a label", line)
            elif set(option) - {"label", "required"} or not isinstance(option.get("required", False), bool):
                problem(path, f"{where} (checkboxes): options may only hold label and required: true / false", line)


def check_form(path: Path, data: object, root: yaml.Node | None, label_names: dict[str, str]) -> dict | None:
    if not isinstance(data, dict):
        problem(path, "must be a mapping with name, description and body")
        return None
    for key in set(data) - TOP_LEVEL_KEYS:
        problem(path, f"unknown top-level key {key!r}")
    for key in ("name", "description"):
        if not is_text(data.get(key)):
            problem(path, f"{key} must be non-empty text")
    if "title" in data and not isinstance(data["title"], str):
        problem(path, "title must be text")

    for label in as_list(data.get("labels")):
        if not isinstance(label, str) or label.lower() not in label_names:
            problem(path, f"label {label!r} isn't in .github/labels.json (GitHub skips labels the repository doesn't have)")
    for project in as_list(data.get("projects")):
        if not isinstance(project, str) or not PROJECT_KEY_PATTERN.match(project):
            problem(path, f"project {project!r} must be OWNER/NUMBER, for example Coalition-of-Freeware-Developers/2")
    for assignee in as_list(data.get("assignees")):
        if not isinstance(assignee, str) or not re.match(r"^[A-Za-z0-9-]+$", assignee):
            problem(path, f"assignee {assignee!r} must be a GitHub user name")

    body = data.get("body")
    if not isinstance(body, list) or not body:
        problem(path, "body must be a non-empty list")
        return data
    lines = body_lines(root)
    ids: set[str] = set()
    labels: set[str] = set()
    for index, element in enumerate(body):
        check_element(path, lines[index] if index < len(lines) else None, index, element, ids, labels)
    if all(isinstance(element, dict) and element.get("type") == "markdown" for element in body):
        problem(path, "body needs at least one field that isn't markdown")
    return data


def check_config(path: Path, data: object) -> None:
    if not isinstance(data, dict):
        problem(path, "must be a mapping")
        return
    for key in set(data) - {"blank_issues_enabled", "contact_links"}:
        problem(path, f"unknown key {key!r}")
    if "blank_issues_enabled" in data and not isinstance(data["blank_issues_enabled"], bool):
        problem(path, "blank_issues_enabled must be true or false")
    for index, link in enumerate(data.get("contact_links") or []):
        if not isinstance(link, dict) or set(link) != {"name", "url", "about"} or not all(is_text(link.get(key)) for key in ("name", "url", "about")):
            problem(path, f"contact_links[{index}]: needs exactly name, url and about, all non-empty text")
        elif not link["url"].startswith("https://"):
            problem(path, f"contact_links[{index}]: url must start with https://")


def form_fields(forms: dict[Path, dict]) -> dict[str, list[tuple[Path, dict, set[str]]]]:
    """Field label -> (form, field, the form's labels) for every non-markdown field."""
    fields: dict[str, list[tuple[Path, dict, set[str]]]] = {}
    for path, data in forms.items():
        form_labels = {label.lower() for label in as_list(data.get("labels")) if isinstance(label, str)}
        for element in data.get("body") or []:
            if isinstance(element, dict) and element.get("type") != "markdown":
                label = (element.get("attributes") or {}).get("label")
                if isinstance(label, str):
                    fields.setdefault(label, []).append((path, element, form_labels))
    return fields


def check_triage(forms: dict[Path, dict], label_names: dict[str, str]) -> None:
    path = GITHUB / "issue-triage.json"
    try:
        config = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        problem(path, f"can't read: {error}")
        return

    new_issues = config.get("newIssues") or {}
    assignees = new_issues.get("assignees", [])
    if not isinstance(assignees, list) or not all(isinstance(name, str) and re.match(r"^[A-Za-z0-9-]+$", name) for name in assignees):
        problem(path, "newIssues.assignees must be a list of GitHub user names")
    project_url = new_issues.get("project")
    if project_url is not None:
        match = PROJECT_URL_PATTERN.match(project_url) if isinstance(project_url, str) else None
        if not match:
            problem(path, "newIssues.project must look like https://github.com/orgs/<org>/projects/<number>")
        else:
            project_key = f"{match.group(1)}/{match.group(2)}"
            for form, data in forms.items():
                if project_key not in as_list(data.get("projects")):
                    problem(form, f"projects: should list {project_key}, the project in issue-triage.json's newIssues.project")

    fields = form_fields(forms)
    for field, mapping in (config.get("fields") or {}).items():
        users = [(form, element) for form, element, _ in fields.get(field, [])]
        if not users:
            problem(path, f'fields["{field}"]: no issue form has a field with this label')
            continue
        offered: set[str] = set()
        for form, element in users:
            if element.get("type") != "dropdown":
                problem(path, f'fields["{field}"]: {relative(form)} uses this label for a {element.get("type")}, not a dropdown')
                continue
            for option in (element.get("attributes") or {}).get("options") or []:
                if not isinstance(option, str):
                    continue  # check_element reports it
                offered.add(option)
                if option not in mapping:
                    problem(form, f'dropdown "{field}": option {option!r} has no entry in .github/issue-triage.json (add it, with [] for no label)')
        for option, labels in mapping.items():
            if option not in offered:
                problem(path, f'fields["{field}"]: option {option!r} isn\'t offered by any form')
            if not isinstance(labels, list):
                problem(path, f'fields["{field}"][{option!r}]: must be a list of labels')
                continue
            for label in labels:
                if not isinstance(label, str) or label.lower() not in label_names:
                    problem(path, f'fields["{field}"][{option!r}]: label {label!r} isn\'t in labels.json')

    for index, check in enumerate(config.get("checks") or []):
        where = f"checks[{index}]"
        rule = check.get("rule")
        check_labels = check.get("labels")
        if rule not in CHECK_RULES:
            problem(path, f"{where}: rule must be one of {sorted(CHECK_RULES)}")
        if not is_text(check.get("ask")):
            problem(path, f"{where}: ask must be non-empty text")
        if not isinstance(check_labels, list) or not check_labels:
            problem(path, f"{where}: labels must be a non-empty list")
            continue
        for label in check_labels:
            if not isinstance(label, str) or label.lower() not in label_names:
                problem(path, f"{where}: label {label!r} isn't in labels.json")
        if rule == "pattern":
            try:
                re.compile(check.get("pattern", ""))
            except (re.error, TypeError) as error:
                problem(path, f"{where}: pattern doesn't compile: {error}")
        if rule == "attachment" and not isinstance(check.get("min", 1), int):
            problem(path, f"{where}: min must be a whole number")
        wanted = {label.lower() for label in check_labels if isinstance(label, str)}
        if not any(form_labels & wanted for _, _, form_labels in fields.get(check.get("field"), [])):
            problem(path, f'{where}: no form labelled {check_labels} has a field "{check.get("field")}", so the check never runs')


def main() -> int:
    label_names = check_labels_file()

    forms: dict[Path, dict] = {}
    names: dict[str, Path] = {}
    template_files = sorted(TEMPLATES.glob("*.yml")) + sorted(TEMPLATES.glob("*.yaml"))
    for path in template_files:
        data, root = load_yaml(path)
        if data is None:
            continue
        if path.stem == "config":
            check_config(path, data)
            continue
        form = check_form(path, data, root, label_names)
        if form is None:
            continue
        forms[path] = form
        name = form.get("name")
        if isinstance(name, str):
            if name in names:
                problem(path, f"name {name!r} is also used by {relative(names[name])}")
            names[name] = path
    for path in sorted(TEMPLATES.glob("*")):
        if path.suffix.lower() not in (".yml", ".yaml", ".md"):
            problem(path, "GitHub ignores this file: issue forms must end in .yml or .yaml")

    check_triage(forms, label_names)

    workflows = sorted(WORKFLOWS.glob("*.yml")) + sorted(WORKFLOWS.glob("*.yaml"))
    for path in workflows:
        load_yaml(path)

    annotate = os.environ.get("GITHUB_ACTIONS") == "true"
    for path, line, message in problems:
        location = f"{relative(path)}:{line}" if line else relative(path)
        print(f"{location}: {message}")
        if annotate:
            line_part = f",line={line}" if line else ""
            print(f"::error file={relative(path)}{line_part}::{message}")

    print(f"Checked {len(forms)} issue form(s), {len(label_names)} label(s) and {len(workflows)} workflow(s): "
          f"{len(problems)} problem(s).")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
