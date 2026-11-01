#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "pyyaml>=6.0.3",
# ]
# ///

import argparse
import dataclasses
import glob
import os
import pathlib
import re
import sys
import yaml


# marks a missing translation; legacy codesets lack "‼"
FIXME = "‼ FIXME ‼"
FIXME_ASCII = "!! FIXME !!"

# languages whose catalogs silently drop what a codeset lacks
# (formerly "iconv -c" in zh_CN/Makefile.am)
LOSSY_LANGS = {"zh_CN"}


@dataclasses.dataclass
class NlsSet:
    id: int
    name: str
    entries: dict[int, dict[str, str]]

    @staticmethod
    def yaml_repr(dumper, data):
        return dumper.represent_mapping('tag:yaml.org,2002:map',
            {
                'name': data.name,
                'id': data.id,
                'entries': data.entries,
            }
        )


@dataclasses.dataclass
class NlsCollection:
    sets: list[NlsSet]
    # per language the codesets of its catalogs; none: one UTF-8
    # catalog without codeset in its name, like for "C"
    codesets: dict[str, list[str]] = dataclasses.field(default_factory=dict)

    @staticmethod
    def yaml_repr(dumper, data):
        return dumper.represent_mapping('tag:yaml.org,2002:map',
            {
                'sets': data.sets,
                'codesets': data.codesets
            }
        )

    def langs(self):
        langs = set()
        for s in self.sets:
            for id, transl in s.entries.items():
                langs.update(transl.keys())
        return langs

    def add_missing(self, langs, symbol = FIXME):
        for set in self.sets:
            for id, trans in set.entries.items():
                for lang in langs:
                    trans.setdefault(lang, symbol)
        return self

    def sort_entries(self):
        self.sets.sort(key=lambda s: s.id)
        for set in self.sets:
            tmp = {}
            for id in sorted(set.entries.keys()):
                trans, trans_sorted = set.entries[id], {}
                for k in sorted(trans.keys()):
                    trans_sorted[k] = trans[k]
                tmp[id] = trans_sorted
            set.entries = tmp
        return self


class NlsFile:

    marker_codeset = "$set "
    marker_codeset_len = len(marker_codeset)

    encodings = {
        "CP1251": "cp1251",
        "ISO-8859-1": "iso8859-1",
        "ISO-8859-15": "iso8859-15",
        "ISO-8859-13": "iso8859-13",
        "ISO-8859-2": "iso8859-2",
        "KOI8-R": "koi8-r",
        "KOI8-U": "koi8-u",
        "UTF-8": "utf-8",
        "eucJP": "euc-jp",
        "utf8": "utf-8",
    }

    # gencat escape sequences, see https://linux.die.net/man/1/gencat
    escapes = {
        "\\": "\\",
        "n": "\n",
        "t": "\t",
        "v": "\v",
        "b": "\b",
        "r": "\r",
        "f": "\f",
    }

    @staticmethod
    def unescape(text: str) -> str:
        # like gencat, an unknown escape drops its backslash
        out, i = [], 0
        while i < len(text):
            c = text[i]
            if c == "\\" and i + 1 < len(text):
                n = text[i + 1]
                if n in NlsFile.escapes:
                    out.append(NlsFile.escapes[n])
                    i += 2
                    continue
                digits = 0
                while digits < 3 and i + 1 + digits < len(text) and text[i + 1 + digits] in "01234567":
                    digits += 1
                if digits > 0:
                    out.append(chr(int(text[i + 1:i + 1 + digits], 8)))
                    i += 1 + digits
                    continue
                out.append(n)
                i += 2
                continue
            out.append(c)
            i += 1
        return "".join(out)

    @staticmethod
    def escape(text: str) -> str:
        for seq, c in NlsFile.escapes.items():
            text = text.replace(c, "\\" + seq)
        return text

    @staticmethod
    def continues(text: str) -> bool:
        # an odd number of trailing backslashes continues the message
        # on the next line, an even number is an escaped backslash
        return (len(text) - len(text.rstrip("\\"))) % 2 == 1

    @staticmethod
    def detect_encoding(filename: str) -> str|None:

        enc = None

        with open(filename, "rb") as file:
            line = file.readline().decode("ascii", errors="ignore")
            if line.startswith("$ codeset="):
                _, enc = line.split("=", 1)
                enc = NlsFile.encodings.get(enc.strip(), None)
                if enc:
                    return enc

        print(f'warning: unknown encoding "{enc}" in {filename}', file=sys.stderr)
        return None

    @staticmethod
    def parse(filename:str, sets: list[NlsSet]) -> bool:

        # msgfile format, see https://linux.die.net/man/1/gencat

        encoding = NlsFile.detect_encoding(filename)

        if not encoding:
            return False

        with open(filename, 'r', encoding=encoding) as file:

            print(f"info: parsing {filename}", file=sys.stderr)

            next(file) # skip encoding line

            # text: the raw message, not None while it continues on the next line
            tid, last_tid, text = None, 0, None
            nls_set = NlsSet(0, "", {})

            cur_transl = str(pathlib.Path(filename).parent)

            for line in file:

                line = line.rstrip('\n')

                # a continuation / multiline entry
                if text is not None:
                    text += line

                # an empty line
                elif line.strip() == "":
                    continue

                # a new set
                elif line.startswith(NlsFile.marker_codeset):

                    tid, last_tid = None, 0
                    if (nls_set.id and len(nls_set.entries) > 0):
                        NlsFile.merge(sets, nls_set, filename)

                    # parse new code-set, the "#name" comment is optional
                    id, _, name = line[NlsFile.marker_codeset_len:].partition("#")
                    nls_set = NlsSet(int(id), name.strip(), {})
                    continue

                # a normal line
                else:
                    name, text = re.match(r"(\S*)[ \t]?(.*)", line).groups()
                    if name.isdigit():
                        tid = int(name)
                    else:
                        # gencat numbers a symbolic name as the next message
                        tid = last_tid + 1
                        print(f'warning: symbolic message name "{name}" taken as {tid} ({filename})', file=sys.stderr)
                    last_tid = tid
                    nls_set.entries.setdefault(tid, {})

                if NlsFile.continues(text):
                    text = text[:-1]
                    continue

                nls_set.entries[tid][cur_transl] = NlsFile.unescape(text)
                text = None

            # the last message continues past the end of the file
            if text is not None:
                nls_set.entries[tid][cur_transl] = NlsFile.unescape(text)

            NlsFile.merge(sets, nls_set, filename)

        return True

    @staticmethod
    def merge(sets: list[NlsSet], nls_set: NlsSet, filename: str):

        # merge nls_set into the previously parsed set with the same id
        set = None
        for s in sets:
            if (s.id == nls_set.id):
                set = s
                break
        if not set:
            sets.append(nls_set)
            return

        if not set.name:
            set.name = nls_set.name

        len_set = len(set.entries)
        len_nls_set = len(nls_set.entries)

        if len_set < len_nls_set:
            print(f"info: mismatch number entries for set {nls_set.name}: {len(set.entries)} vs {len_nls_set} ({filename})",
                  file=sys.stderr)

        for id, transl in nls_set.entries.items():
            if id in set.entries:
                set.entries[id] |= transl
            else:
                set.entries[id] = transl

    @staticmethod
    def write(filename: str, lang: str, ncoll: NlsCollection, codeset: str = "UTF-8") -> bool:

        errors = "strict"
        if lang in LOSSY_LANGS:
            errors = "ignore"

        lines = [f"$ codeset={codeset}\n"]
        for set in ncoll.sets:
            lines.append("\n")
            lines.append(f"$set {set.id} #{set.name}\n")
            lines.append("\n")
            for id, transl in set.entries.items():
                transl = transl.get(lang, "FIXME")
                if codeset != "UTF-8":
                    transl = transl.replace(FIXME, FIXME_ASCII)
                transl = NlsFile.escape(transl)
                lines.append(f"{id} {transl}\n")

        data = []
        for line in lines:
            try:
                data.append(line.encode(codeset, errors=errors))
            except UnicodeEncodeError as e:
                print(f'error: {codeset} lacks "{e.object[e.start:e.end]}" in "{line.rstrip()}" ({filename})',
                      file=sys.stderr)
                return False

        with open(filename, "wb") as f:
            print(f"info: write to {filename}", file=sys.stderr)
            f.write(b"".join(data))
        return True

    @staticmethod
    def generated_name(codeset: str) -> str:
        return f"generated-{codeset}.m"

    @staticmethod
    def codesets(lang_dir: str) -> list[str]:

        # the codesets of the generated-<codeset>.m files in lang_dir
        names = [p.name for p in pathlib.Path(lang_dir).glob("generated-*.m")]
        return sorted(n[len("generated-"):-len(".m")] for n in names)


def write_catalogs_am(filename: str, codesets: dict[str, list[str]]) -> bool:

    # one catalog per language and codeset, installed as
    # $(LOCALE_PATH)/<lang>.<codeset>/fluxbox.cat; a language without
    # codesets is installed as $(LOCALE_PATH)/<lang>/fluxbox.cat
    catalogs = []
    for lang in sorted(codesets):
        if not codesets[lang]:
            catalogs.append((lang, f"nls/{lang}/generated.m"))
        for codeset in codesets[lang]:
            catalogs.append((f"{lang}.{codeset}", f"nls/{lang}/{NlsFile.generated_name(codeset)}"))

    out = [
        "# generated by nls/fluxbox-nls.py from nls/fluxbox-nls.yaml, do not edit\n",
        "\n",
        "nls_MFILES =",
    ]
    for _, mfile in catalogs:
        out.append(f" \\\n\t{mfile}")
    out.append("\n\nnls_CATFILES =")
    for dir, _ in catalogs:
        out.append(f" \\\n\tnls/{dir}/fluxbox.cat")
    out.append("\n\nnls_CATDIRS =")
    for dir, _ in catalogs:
        out.append(f" \\\n\t{dir}")
    out.append("\n\nCLEANFILES += $(nls_CATFILES)\n")

    out.append("\nif NLS_CATALOGS\n")
    for dir, _ in catalogs:
        var = "nls_" + re.sub(r"[^A-Za-z0-9_]", "_", dir)
        out.append(f"{var}dir = $(LOCALE_PATH)/{dir}\n")
        out.append(f"{var}_DATA = nls/{dir}/fluxbox.cat\n")
    out.append("endif\n")

    for dir, mfile in catalogs:
        out.append(f"\nnls/{dir}/fluxbox.cat: {mfile}\n")
        out.append(f"\t$(nls_gencat) $(srcdir)/{mfile}\n")

    with open(filename, "w", encoding="utf-8") as f:
        print(f"info: write to {filename}", file=sys.stderr)
        f.write("".join(out))
    return True


def create_yaml():

    # reads back the generated files: per language the UTF-8 one
    # (generated-UTF-8.m, or generated.m for a language without codesets)

    yaml_tag_str = 'tag:yaml.org,2002:str'
    def str_repr(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar(yaml_tag_str, data, style='|')
        return dumper.represent_scalar(yaml_tag_str, data)
    yaml.add_representer(str, str_repr)
    yaml.add_representer(NlsSet, NlsSet.yaml_repr)
    yaml.add_representer(NlsCollection, NlsCollection.yaml_repr)

    ncoll = NlsCollection([])

    translations = []
    for lang_dir in glob.glob("./*/"):
        for name in ("generated-UTF-8.m", "generated.m"):
            translation = os.path.join(lang_dir, name)
            if os.path.exists(translation):
                translations.append(translation)
                break
    for translation in translations:
        NlsFile.parse(translation, ncoll.sets)

    for translation in sorted(translations):
        lang_dir = pathlib.Path(translation).parent
        ncoll.codesets[lang_dir.name] = NlsFile.codesets(lang_dir)

    ncoll.add_missing(ncoll.langs())
    ncoll.sort_entries()

    result = yaml.dump(ncoll,
                       explicit_start=True,
                       sort_keys=False,
                       allow_unicode=True)
    print(result)

def create_translation_files(filename: str, out_folder: str) -> bool:
    with open(filename, "r", encoding="utf-8") as file:
        data = yaml.safe_load(file)
        for key in ("codesets", "sets"):
            if key not in data:
                print(f'error: no "{key}" attribute in "{filename}"', file=sys.stderr)
                return False
        ncoll = NlsCollection([], data["codesets"])
        for set in data["sets"]:
            ncoll.sets.append(NlsSet(int(set["id"]), set["name"], set["entries"]))
        langs = ncoll.langs()
        for lang in sorted(langs - ncoll.codesets.keys()):
            print(f'error: no codesets for "{lang}" in "{filename}"', file=sys.stderr)
            return False
        ncoll.sort_entries()
        ok = True
        for lang in sorted(langs):
            lang_dir = os.path.join(out_folder, lang)
            os.makedirs(lang_dir, exist_ok=True)
            names = {NlsFile.generated_name(codeset): codeset for codeset in ncoll.codesets[lang]}
            if not names:
                names["generated.m"] = "UTF-8"
            for stale in pathlib.Path(lang_dir).glob("generated*.m"):
                if stale.name not in names:
                    print(f"info: remove {stale}", file=sys.stderr)
                    stale.unlink()
            for name, codeset in names.items():
                ok &= NlsFile.write(os.path.join(lang_dir, name), lang, ncoll, codeset)
        if ok:
            ok = write_catalogs_am(os.path.join(out_folder, "catalogs.am"),
                                   {lang: ncoll.codesets[lang] for lang in langs})
        return ok


def main():
    parser = argparse.ArgumentParser(
        prog="fluxbox-nls",
        description="fluxbox nls utility",
    )
    parser.add_argument("mode")
    parser.add_argument("--in-yaml", type=str, action="store")
    parser.add_argument("--out-folder", type=str, action="store")

    args = parser.parse_args()

    if args.mode == "create-yaml":
        create_yaml()
    elif args.mode == "create-translation-files":
        if not create_translation_files(args.in_yaml, args.out_folder):
            sys.exit(1)
    else:
        print(f'unknown mode "{args.mode}"')

if __name__ == "__main__":
    main()
