# OpenThread Documentation Style Guide

OpenThread documentation lives in two locations:

- **GitHub** — All guides and tutorials across the complete [OpenThread organization](https://github/openthread).
- **openthread.io** - OpenThread news and features, educational material, API reference, and select guides and tutorials found on GitHub.

All documentation contributions are done on GitHub and mirrored on [openthread.io](https://openthread.io), and will be reviewed for clarity, accuracy, spelling, and grammar prior to acceptance.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for general information on contributing to this project.

## Location

Place all documentation contributions in the appropriate location in the [`/doc/site/[locale]/`](./site) directory. Contributions should be added in a location that mirrors the structure of the openthread.io documentation site. Browse that site and use the URL paths to determine the proper directory.

If you are unsure of the best location for your contribution, create an Issue and ask, or let us know in your Pull Request.

### Updating the site menus

When adding a new document, or moving one, also update the `_toc.yaml` file(s) in the related folders. For example, the `/doc/site/en/guides/_toc.yaml` file would be the menu for the [Guides section on openthread.io](https://openthread.io/guides). If a menu doesn't yet exist on GitHub, let the OpenThread Team know in a new Issue we will add it.

New documents should be added to existing site menu TOCs where appropriate. If you are unsure of where to place a document within the menu, let us know in your Pull Request.

## Style

OpenThread follows the [Google Developers Style Guide](https://developers.google.com/style/). See the [Highlights](https://developers.google.com/style/highlights) page for a quick overview.

## Links

For consistency, all document links should point to the content on GitHub, unless it refers to content only on [openthread.io](https://openthread.io).

The text of a link should be descriptive, so it's clear what the link is for:

> For more information, see the [OpenThread Style Guide](./STYLE_GUIDE.md).

## Markdown guidelines

Use standard Markdown when authoring OpenThread documentation. While HTML may be used for more complex content such as tables, use Markdown as much as possible. To ease mirroring and to keep formatting consistent with openthread.io, we ask that you follow the specific guidelines listed here.

> Note: Edit this file to see the Markdown behind the examples.

### Headers

The document title should be an h1 header (#) and in title case (all words are capitalized). All section headers should be h2 (##) or lower and in sentence case (only the first word and proper nouns are capitalized).

The best practice for document clarity is to not go lower than h3, but h4 is fine on occasion. Try to avoid using h4 often, or going lower than h4. If this happens, the document should be reorganized or broken up to ensure it stays at h3 with the occasional h4.

### Command line examples

Feel free to use either `$` or `%` to preface command line examples, but be consistent within the same doc or set of docs:

```
$ git clone https://github.com/openthread/openthread.git
% git clone https://github.com/openthread/openthread.git
```

### OpenThread CLI prompt

Use the OpenThread CLI prompt for all CLI-related commands. The prompt is `>`:

```
> ifconfig up
```

### Other prompts

If you need use a **full terminal prompt** with username and hostname, use the format of `root@{hostname}{special-characters}#`.

For example, when logged into a Docker container, you might have a prompt like this:

```
root@c0f3912a74ff:/#
```

### Commands and output

All example commands and output should be in code blocks with backticks:

```
code in backticks
```

...unless the code is within a step list. In a step list, indent the code blocks:

    code indented

### Code blocks in step lists

When writing procedures that feature code blocks, indent the content for the code blocks:

1.  Step one:

        $ git clone https://github.com/openthread/openthread.git
        $ cd openthread

1.  Step two, do something else:

        $ ./configure

For clarity in instructions, avoid putting additional step commands after a code sample within a step item. Instead rewrite the instruction so this is not necessary.

For example, avoid this:

1.  Step three, do this now:

        $ ./configure

    And then you will see that thing.

Instead, do this:

1.  Step three, do this now, and you will see that thing:

        $ ./configure

### Inline code

Use backticks for `inline code`. This includes file paths and file or binary names.

### Replaceable variables

In code examples, denote a replaceable variable with curly braces:

```
make -f examples/Makefile-{platform}
```

### Step header numbers

If you want your headers to render with nice-looking step numbers on openthread.io, use level 2 Markdown headers in this format:

    ## Step 1: Text of the step
    ## Step 2: Text of the next step
    ## Step 3: Text of the last step

### Callouts

Use a blockquote `>` with one of these callout types:

-     Note
- Caution
- Warning

For example:

> Note: This is something to be aware of.

Or:

> Caution: The user should be careful running the next command.
