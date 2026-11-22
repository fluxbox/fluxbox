#!/bin/sh
# Show texts with glyphs the main font lacks, to check the Xft font fallback.
#
#   src/tests/font_fallback.sh [font] [orient|all]
#
#   font    Xft font name, default "sans-10"
#   orient  0..3 (testFont -orient), or "all" for four windows at once
#
# Needs a built ./testFont (make testFont); override with TESTFONT=path.
# Missing glyphs show as boxes; with the fallback they render from other fonts.

font=${1:-sans-10}
orient=${2:-0}
testfont=${TESTFONT:-./testFont}

# testFont only takes the utf8 path in a UTF-8 locale; C.UTF-8 is built into
# glibc >= 2.35, so it works even where LOCALE_ARCHIVE is missing (nix-shell)
export LC_ALL=C.UTF-8

show() {
    "$testfont" -orient "$1" \
        "ascii only: The quick brown fox|$font" \
        "emojidb title: Title Emojis | 𖥻 ׁ ׅ title ! ﹒ ◡◡𓄲 𓏼 name 𓈒 !𐔌 . ⋮ name .ᐟ ֹ ₊ ꒱ּ֯. ❥ ּ֯ ┆꒰ 𝗍౿𝗑𝗍 .ᐟ ꒱|$font" \
        "symbols: ◡ ⋮ ❥ ┆ ₊ ﹒|$font" \
        "bamum: 𖥻|$font" \
        "hieroglyphs: 𓄲 𓏼 𓈒|$font" \
        "elbasan: 𐔌|$font" \
        "yi: ꒰ ꒱|$font" \
        "telugu: ౿|$font" \
        "math sans: 𝗍𝗑𝗍|$font" \
        "hebrew marks: אׁ בּ וֹ|$font" \
        "cjk: 日本語 中文 한국어|$font" \
        "emoji: 😀 🎉 🐧|$font" \
        "mixed runs: a𓄲b𓏼c日d😀e|$font" \
        "truncated (resize the window, the end must stay clean): ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀 ä日𓄲ö本𓏼ü語😀|$font"
}

if [ "$orient" = all ]; then
    for o in 0 1 2 3; do
        show "$o" &
    done
    wait
else
    show "$orient"
fi
