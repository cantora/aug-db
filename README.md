## aug-db 

aug-db is a plugin for [cantora/aug](https://github.com/cantora/aug) which
provides interactive access to a sqlite database within the context of the
terminal. The general idea is to provide something like the bash history
search but with richer features.

## installation
aug-db depends on libccan and sqlite, but the make file will automatically
download both and compile them for linking against aug-db. The make file
will use `git` to download libccan and will use `curl` to download sqlite.

To build the plugin, simply run `make` in the root of the source tree. If
successful, you will find the `aug-db.so` shared library in the root 
directory of the source tree. Simply copy that file to your configured
aug plugin directory and add an `[aug-db]` section to your aug configuration
file (aug will not load a plugin unless it finds a section for it in the
configuration file).

## configuration
You can configure the following settings in the `[aug-db]` section of your
aug configuration file:
 * **db**:     setting **db** to a file path will cause aug-db to look for 
               your sqlite database file at the given file path instead of
               the default "$HOME/.aug-db.sqlite".
 * **key**:    setting **key** to a valid aug key name string will configure
               the command key extension. The default extension used by 
               aug-db is `^R`. Run `aug --char-rep` to see a list of key name
               strings that aug understands.

