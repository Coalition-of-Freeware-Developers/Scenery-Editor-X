"""
Tests of tools/i18n/sedx_i18n.py (standard library only):

    python -I -m unittest discover -s tools/i18n -p "test_*.py"
"""

import importlib.util
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
SPEC = importlib.util.spec_from_file_location("sedx_i18n", os.path.join(HERE, "sedx_i18n.py"))
tool = importlib.util.module_from_spec(SPEC)
sys.modules["sedx_i18n"] = tool  # dataclasses look their module up while the class is made
SPEC.loader.exec_module(tool)


class SourceTree:
    """A temporary repository with source/ files, pointed at by the tool's paths while in use."""

    def __init__(self, files):
        self.files = files

    def __enter__(self):
        self.directory = tempfile.TemporaryDirectory()
        root = self.directory.name
        for name, text in self.files.items():
            path = os.path.join(root, "source", name)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as handle:
                handle.write(text)
        self.saved = (tool.ROOT, tool.SOURCE_DIR, tool.EXCLUDED_DIRS)
        tool.ROOT = root
        tool.SOURCE_DIR = os.path.join(root, "source")
        tool.EXCLUDED_DIRS = {os.path.normcase(os.path.join(root, "source", "Tests"))}
        return self

    def __exit__(self, *exc):
        tool.ROOT, tool.SOURCE_DIR, tool.EXCLUDED_DIRS = self.saved
        self.directory.cleanup()


HELPERS = """
namespace SceneryEditorX::EditorUI
{
	// TRANSLATORS: A panel title, in capitals
	bool BeginPanel(Text name, bool *open = nullptr);
	void PropertyLabel(const Text &label);
	std::string IconLabel(const char *icon, Text label);
	bool BeginPropertyGrid(const char *id, float width = 120.0f);
}
"""


class LexerTests(unittest.TestCase):
    def test_strings_comments_and_preprocessor(self):
        tokens = tool.code_tokens(tool.tokenize(
            '#define X Tr("not this")\n'
            '// Tr("nor this")\n'
            '/* Tr("nor\n this") */\n'
            'Tr("a" "b" R"(c"d)" u8"e");\n'
            "char q = '\"'; Tr(\"\\x41\\101\\n\\u00e9\");\n"))
        strings = [t for t in tokens if t.kind == "str"]
        self.assertEqual([s.value for s in strings], ["a", "b", 'c"d', "e", "AA\né"])
        self.assertEqual(strings[2].prefix, "R")
        self.assertEqual(strings[3].prefix, "u8")
        self.assertEqual(strings[0].line, 5)     # After the two-line block comment

    def test_identifier_ending_in_prefix_letter(self):
        tokens = tool.code_tokens(tool.tokenize('AddU"x"'))
        self.assertEqual([t.kind for t in tokens], ["id", "str"])


class ExtractTests(unittest.TestCase):
    def extract(self, files):
        with SourceTree(files):
            catalog = tool.extract()
            text = tool.template_text(catalog)
            again = tool.template_text(tool.extract())
        self.assertEqual(text, again, "the template must be the same every time")
        return catalog, text

    def test_keywords_contexts_plurals_and_comments(self):
        catalog, text = self.extract({"Editor/a.cpp": HELPERS + """
void Draw()
{
	ImGui::TextUnformatted(Tr("Ready"));
	// TRANSLATORS: {0} is a count, {1} a folder
	status = TrNF("{0} file in {1}", "{0} files in {1}", n, folder);
	ImGui::Button(TrL("Export##pack"));
	ImGui::MenuItem(TrL(Ctx("verb", "Open")));
	label = TrF(Ctx("unit", "{} m"), metres);
	static constexpr Msg KINDS[] = { TrNoop("Runway"), Ctx("surface", "Grass") };
	text = Tr(KINDS[0]);                      // A Msg variable: nothing to extract here
	ImGui::Text("%s", TrN("tree", "trees", count));
	Tr(Ctx("state", "Open"));
	EditorUI::PropertyLabel(first ? Text("End 1") : Text("End 2"));
	static constexpr Msg ALONE = Msg("Alone");
}
"""})
        self.assertEqual(catalog.errors, [])
        keys = set(catalog.entries)
        self.assertEqual(keys, {(None, "Ready"), (None, "{0} file in {1}"), (None, "Export"), ("verb", "Open"), ("unit", "{} m"),
                                (None, "Runway"), ("surface", "Grass"), (None, "tree"), ("state", "Open"), (None, "End 1"),
                                (None, "End 2"), (None, "Alone")})
        files = catalog.entries[(None, "{0} file in {1}")]
        self.assertEqual(files.plural, "{0} files in {1}")
        self.assertEqual(files.comments, ["{0} is a count, {1} a folder"])
        self.assertIn("python-brace-format", files.flags)
        self.assertIn("python-brace-format", catalog.entries[("unit", "{} m")].flags)
        self.assertNotIn("python-brace-format", catalog.entries[(None, "Ready")].flags)
        self.assertEqual(catalog.entries[(None, "tree")].plural, "trees")
        self.assertIn('msgid_plural "{0} files in {1}"\nmsgstr[0] ""\nmsgstr[1] ""', text)
        self.assertIn('#: source/Editor/a.cpp', text)

    def test_text_parameters_are_found_from_declarations(self):
        catalog, text = self.extract({
            "Editor/editor_ui.h": HELPERS,
            "Editor/panel.cpp": """
void Panel()
{
	if (!EditorUI::BeginPanel("PROPERTIES###Properties")) return;
	EditorUI::PropertyLabel("Heading");
	EditorUI::PropertyLabel(name.c_str());      // Run-time text
	EditorUI::PropertyLabel(Ctx("angle", "Pitch"));
	ImGui::MenuItem(EditorUI::IconLabel(ICON, "Delete").c_str());
	EditorUI::BeginPropertyGrid("##grid", 100.0f);  // Not a Text parameter
	ImGui::Begin("Not extracted");                   // Lint's business, not extract's
}
"""})
        self.assertEqual(set(catalog.entries), {(None, "PROPERTIES"), (None, "Heading"), ("angle", "Pitch"), (None, "Delete")})
        self.assertEqual(catalog.entries[(None, "PROPERTIES")].comments, ["A panel title, in capitals"])

    def test_tests_and_vendored_code_are_skipped(self):
        catalog, _ = self.extract({"Tests/t.cpp": 'Tr("test only");', "Editor/b.cpp": 'Tr("kept");'})
        self.assertEqual(set(catalog.entries), {(None, "kept")})

    def test_conflicting_plurals_are_errors(self):
        catalog, _ = self.extract({"Editor/c.cpp": 'Tr("file"); TrN("file", "files", n); Tr("##only-id"); Tr("");'})
        self.assertEqual(len(catalog.errors), 1)
        self.assertNotIn((None, ""), catalog.entries)

    def test_multiline_strings_are_split_after_line_breaks(self):
        self.assertEqual(tool.po_string("msgid", "one\ntwo"), 'msgid ""\n"one\\n"\n"two"')
        self.assertEqual(tool.po_string("msgid", "one\n"), 'msgid "one\\n"')
        self.assertEqual(tool.po_string("msgstr", 'say "hi"\t\\'), 'msgstr "say \\"hi\\"\\t\\\\"')


class PoTests(unittest.TestCase):
    def test_parse_and_format_round_trip(self):
        text = ('# Comment\nmsgid ""\nmsgstr ""\n"Language: de\\n"\n"Plural-Forms: nplurals=2; plural=(n != 1);\\n"\n\n'
                '# translator\n#. extracted\n#: source/x.cpp\n#, fuzzy, python-brace-format\nmsgctxt "c"\nmsgid "{} a"\nmsgid_plural "{} as"\n'
                'msgstr[0] "{} A"\nmsgstr[1] "{} As"\n\n#~ msgid "old"\n#~ msgstr "alt"\n')
        po = tool.parse_po(text)
        self.assertEqual(po.errors, [])
        self.assertEqual(po.get_header("language"), "de")
        self.assertEqual(len(po.entries), 2)
        entry = po.entries[0]
        self.assertEqual((entry.context, entry.msgid, entry.plural, entry.msgstr), ("c", "{} a", "{} as", ["{} A", "{} As"]))
        self.assertTrue(entry.fuzzy)
        self.assertTrue(po.entries[1].obsolete)
        self.assertEqual(tool.parse_po(tool.format_po(po, 2)).entries[0].msgstr, ["{} A", "{} As"])

    def test_errors_have_lines(self):
        po = tool.parse_po('msgid "a"\nmsgstr "b\n\nbogus "x"\n')
        self.assertEqual([e.split(":")[0] for e in po.errors], ["line 2", "line 4"])

    def test_brace_fields(self):
        self.assertEqual(tool.brace_fields("{} of {}"), [(0, ""), (1, "")])
        self.assertEqual(tool.brace_fields("{1:.2f} {0}"), [(1, ".2f"), (0, "")])
        self.assertEqual(tool.brace_fields("{{x}}"), [])
        self.assertIsNone(tool.brace_fields("{} {1}"))
        self.assertIsNone(tool.brace_fields("{"))


class CatalogTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.languages = tool.load_languages()

    def tearDown(self):
        self.directory.cleanup()

    def write(self, name, body, header="Language: de\\nPlural-Forms: nplurals=2; plural=(n != 1);\\n"):
        path = os.path.join(self.directory.name, name)
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(f'msgid ""\nmsgstr "{header}"\n\n' + body)
        return path

    def check(self, path, template_keys=None):
        errors, warnings, stats, _ = tool.check_catalog(path, template_keys, self.languages)
        return errors, warnings, stats

    def test_a_good_catalog_passes(self):
        path = self.write("de.po", '#, python-brace-format\nmsgid "{0} of {1}"\nmsgstr "{1}: {0}"\n\n'
                                   'msgid "Save"\nmsgstr "Speichern"\n\nmsgid "Later"\nmsgstr ""\n')
        errors, warnings, stats = self.check(path)
        self.assertEqual(errors, [])
        self.assertEqual(stats, {"total": 3, "translated": 2, "fuzzy": 0})

    def test_each_rule_finds_its_defect(self):
        cases = {
            "field": ('#, python-brace-format\nmsgid "{} items"\nmsgstr "{1} Dinge"\n', "field {1}"),
            "spec": ('#, python-brace-format\nmsgid "{:.2f} m"\nmsgstr "{:.1f} m"\n', "formats field"),
            "missing field": ('#, python-brace-format\nmsgid "{} of {}"\nmsgstr "{}"\n', "leaves out"),
            "malformed": ('#, python-brace-format\nmsgid "{} x"\nmsgstr "{ x"\n', "malformed"),
            "id marker": ('msgid "Open"\nmsgstr "Offen##x"\n', "'##'"),
            "spacing": ('msgid "Name:"\nmsgstr "Name: "\n', "spacing"),
            "plural count": ('msgid "a"\nmsgid_plural "as"\nmsgstr[0] "x"\n', "plural forms"),
            "duplicate": ('msgid "a"\nmsgstr "b"\n\nmsgid "a"\nmsgstr "c"\n', "twice"),
            "max length": ('#, max-length:5\nmsgid "Short"\nmsgstr "Sehr lang"\n', "longer than 5"),
        }
        for name, (body, expected) in cases.items():
            with self.subTest(name):
                errors, _, _ = self.check(self.write("de.po", body))
                self.assertTrue(any(expected in e for e in errors), f"{name}: {errors}")

    def test_header_checks(self):
        errors, _, _ = self.check(self.write("de.po", "", header="Language: fr\\nPlural-Forms: nplurals=2; plural=(n > 1);\\n"))
        self.assertTrue(any("file name says de" in e for e in errors))
        errors, _, _ = self.check(self.write("ru.po", "", header="Language: ru\\nPlural-Forms: nplurals=2; plural=(n != 1);\\n"))
        self.assertTrue(any("nplurals=2" in e for e in errors))
        errors, _, _ = self.check(self.write("xx.po", "", header="Language: xx\\n"))
        self.assertTrue(any("language table" in e for e in errors))
        self.assertTrue(any("Plural-Forms" in e for e in errors))

    def test_stale_entries_are_warnings(self):
        path = self.write("de.po", 'msgid "Gone"\nmsgstr "Weg"\n')
        errors, warnings, _ = self.check(path, template_keys={(None, "Here")})
        self.assertEqual(errors, [])
        self.assertTrue(any("isn't in the template" in w for w in warnings))

    def test_update_keeps_translations_and_obsoletes_the_rest(self):
        template = tool.parse_po('msgid ""\nmsgstr ""\n\n#: a.cpp\nmsgid "Save"\nmsgstr ""\n\nmsgid "New"\nmsgstr ""\n\n'
                                 'msgid "{} file"\nmsgid_plural "{} files"\nmsgstr[0] ""\nmsgstr[1] ""\n')
        po = tool.parse_po('msgid ""\nmsgstr "Language: ru\\n"\n\n# keep me\nmsgid "Save"\nmsgstr "Sokhranit"\n\n'
                           'msgid "Removed"\nmsgstr "Udaleno"\n\nmsgid "Empty"\nmsgstr ""\n')
        merged = tool.merge_catalog(po, template, 3)
        by_id = {e.msgid: e for e in merged.entries}
        self.assertEqual(by_id["Save"].msgstr, ["Sokhranit"])
        self.assertEqual(by_id["Save"].translator_comments, ["keep me"])
        self.assertEqual(by_id["Save"].references, ["a.cpp"])
        self.assertEqual(by_id["New"].msgstr, [""])
        self.assertEqual(by_id["{} file"].msgstr, ["", "", ""])
        self.assertTrue(by_id["Removed"].obsolete)
        self.assertNotIn("Empty", by_id)
        text = tool.format_po(merged, 3)
        self.assertIn('#~ msgid "Removed"', text)
        self.assertEqual(tool.parse_po(text).errors, [])


class ChangesTests(unittest.TestCase):
    def test_added_and_removed_messages(self):
        old = 'msgid ""\nmsgstr ""\n\n#: a.cpp\nmsgid "Keep"\nmsgstr ""\n\n#: a.cpp\nmsgid "Gone"\nmsgstr ""\n'
        new = ('msgid ""\nmsgstr ""\n\n#: a.cpp\nmsgid "Keep"\nmsgstr ""\n\n#: b.cpp\nmsgctxt "verb"\nmsgid "Open | close"\nmsgstr ""\n\n'
               '#: b.cpp\nmsgid "{} file"\nmsgid_plural "{} files"\nmsgstr[0] ""\nmsgstr[1] ""\n')
        summary = tool.describe_changes(old, new)
        self.assertIn("2 new and 1 removed message(s); the template has 3.", summary)
        self.assertIn("| Open \\| close | verb | b.cpp |", summary)
        self.assertIn("{} file / {} files", summary)
        self.assertIn("| Gone |  | a.cpp |", summary)
        self.assertIn("0 new and 0 removed", tool.describe_changes(new, new))
        self.assertIn("3 new", tool.describe_changes("", new))


class TernaryLintTests(unittest.TestCase):
    def test_literal_ternaries_passed_to_text_parameters(self):
        with SourceTree({"Editor/ui.h": HELPERS, "Editor/panel.cpp": """
void Panel(bool a, const char *name)
{
	EditorUI::PropertyLabel(a ? "End 1" : "End 2");                 // Flagged
	EditorUI::PropertyLabel(a ? "One" : "Two" " parts");            // Flagged (adjacent literals)
	EditorUI::IconLabel(ICON, a ? "Pause" : "Play");                // Flagged (second parameter)
	EditorUI::PropertyLabel(a ? Text("End 1") : Text("End 2"));     // Right
	EditorUI::PropertyLabel(a ? name : "Default");                  // Run-time text: not this rule
	EditorUI::PropertyLabel(Tr(a ? KINDS[0] : KINDS[1]));           // Msg values
	EditorUI::BeginPropertyGrid(a ? "##a" : "##b", 1.0f);           // Not a Text parameter
	ImGui::Text(a ? "x" : "y");                                     // Not a Text function
	foo(f(a ? "p" : "q"), 1);                                       // Not a Text function
}
"""}):
            findings = tool.ternary_findings()
        self.assertEqual([(line, function) for _, line, function in findings],
                         [(4, "PropertyLabel"), (5, "PropertyLabel"), (6, "IconLabel")])


class LintTests(unittest.TestCase):
    def test_needs_translation(self):
        self.assertTrue(tool.needs_translation("Save"))
        self.assertTrue(tool.needs_translation("Open##recent"))
        self.assertFalse(tool.needs_translation("##zoom"))
        self.assertFalse(tool.needs_translation("%.2f m"))
        self.assertFalse(tool.needs_translation("X"))
        self.assertFalse(tool.needs_translation("{} / {}"))
        self.assertFalse(tool.needs_translation("%s"))

    def test_language_table_is_read_from_the_cpp(self):
        languages = tool.load_languages()
        self.assertEqual(languages["ja"]["nplurals"], 1)
        self.assertEqual(languages["ja"]["native"], "日本語")
        self.assertEqual(languages["ru"]["nplurals"], 3)
        self.assertEqual(tool.normalize_tag("pt-br"), "pt_BR")
        self.assertEqual(tool.normalize_tag("zh-hans"), "zh_Hans")


if __name__ == "__main__":
    unittest.main()
