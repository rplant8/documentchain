Translations
============

The DMS Core project has been designed to support multiple localisations. This makes adding new phrases, and completely new languages easily achievable.

### Translation process
In DMS Core, unlike Bitcoin, we currently do not use a translation platform. The translation is done manually in the .ts files:
1. Add new phrases to the *.ts files in `src/qt/locale`.
1. Give the language files to the appropriate translators.

`src/qt/locale/dms_en.ts` is treated in a special way. It is used as the source for all other translations.

#### Example of the main file "dms_en.ts"
```
...
</context>
<context>
    <name>DocumentList</name>
    <message>
        <location filename="../forms/documentlist.ui"/>
        <source>Document Revision</source>
        <translation>Document Revision</translation>
    </message>
    ...
</context>
</TS>
 ```

#### Example of a language file
```
...
</context>
<context>
    <name>DocumentList</name>
    <message>
        <source>Document Revision</source>
        <translation>Dokumentenrevision</translation>
    </message>
    ...
</context>
</TS>
 ```

### Handling Plurals (in source files)
When new plurals are added to the source file, it's important to do the following steps:

1. Open `dms_en.ts` in Qt Linguist (included in the Qt SDK)
2. Search for `%n`, which will take you to the parts in the translation that use plurals
3. Look for empty `English Translation (Singular)` and `English Translation (Plural)` fields
4. Add the appropriate strings for the singular and plural form of the base string
5. Mark the item as done (via the green arrow symbol in the toolbar)
6. Repeat from step 2, until all singular and plural forms are in the source file
7. Save the source file

### Translating a new language
To create a new language template, you will need to edit the languages manifest file `src/qt/dms_locale.qrc` and add a new entry. Below is an example of the English language entry.

```xml
<qresource prefix="/translations">
    <file alias="en">locale/dms_en.qm</file>
    ...
</qresource>
```

**Note:** that the language translation file **must end in `.qm`** (the compiled extension), and not `.ts`.