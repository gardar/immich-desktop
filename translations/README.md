# Translating Immich Desktop

The source language is English. Qt Linguist catalogs (`immich_<locale>.ts`)
contain application text, translator context, source locations and plural forms.
English is complete; German and French currently provide initial translations
for navigation and common actions. Unfinished translations appear in English.

The app follows the operating system's preferred UI languages, trying regional
and script variants before a base language, then falling back to English.
Automatic mode keeps the system's regional date and number formats. Choose a
specific language under **Settings → Appearance → Language** to override it.
Restart the app after changing this setting.

## Update an existing language

Install Qt's **Linguist Tools** alongside the normal build dependencies. On
Debian/Ubuntu, install `qt6-tools-dev`, `qt6-tools-dev-tools`, `qt6-l10n-tools`,
and `qt6-translations-l10n`. The separate `qt6-tools-dev-tools` package supplies
`lprodump`, which Qt's LinguistTools CMake targets also require.

```sh
cmake -S . -B build
cmake --build build --target update_translations
linguist translations/immich_de.ts
cmake --build build
ctest --test-dir build --output-on-failure
```

Use your Qt kit's `linguist` executable if it is not on PATH. Translate messages
and mark them finished in Linguist. Keep placeholders such as `%1`, `%2`, and
`%n`, rich-text markup, and link destinations intact. Fill in every plural form
offered for your language. Review translations in the running app, including
longer labels and dialogs, before committing the `.ts` files.

Normal builds use `lrelease -nounfinished` to compile and embed `.qm` catalogs;
they do not edit source catalogs. Build output and installers include the same
catalogs without requiring loose translation files. Qt's `qtbase` catalogs for
the supported languages are also embedded when supplied by the Qt installation,
and the runtime can additionally use Qt's installed translations.

## Add a language

Create `translations/immich_<locale>.ts` using a Qt locale identifier (for example
`es` or `pt_BR`), with the following initial content, then configure and update:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es" sourcelanguage="en"/>
```

CMake discovers catalogs during configuration; no C++ language list needs to
change. Reconfigure after adding a file. Translated languages appear under their
native names in the language selector. Commit source catalogs, not generated
`.qm` files.

## Adding UI text

Use `tr("Text")` inside a QObject subclass, or
`QCoreApplication::translate("Context", "Text")` in free functions. Keep full
sentences together; use `%1` placeholders rather than concatenating words. Use
`tr("%n item(s)", nullptr, count)` for counts, never English plural suffixes.
Use `QLocale` for displayed dates and numbers. Keep API field names, resource
paths, settings keys, filenames, CSS and other machine identifiers untranslated.
Run `update_translations` when strings change and commit the updated catalogs.
