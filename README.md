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

## usage
To put stuff in your database you can use the `aug-db` script in the script
directory of the source tree. Each item in your database is a blob of text
and a set of tags describing it. For example you could run the following
to save the skeleton of a bash 'for' loop in your database:  
```
$> ./aug-db -i '{ for i in {1..10}; do echo $i; done; }' -t bash 'command line' code 'for loop'
added new blob to db
```  
Here we add an entry with -i flag and store it with a list of tags using the -t
flag. For more usage information from this script run `./aug-db -h`.

After aug has loaded, at any time you can use the command key extension to bring
up the aug-db UI. For example, if your aug command key is `^B` and the aug-db
extension is left as the default `^R`, you can type `^B ^R` and you will see the
aug-db UI. The UI simply shows you a list of top results from your database, and
will filter those results based on any text you type. If you press any non-text
key such as <enter>, `^J`, `^A`, etc... aug-db will insert the text of the 
top-most result into the terminal and exit the UI. It will also insert the 
non-text key you pressed; that is, if you pressed enter the text will be inserted
into the terminal followed by an enter key. This is similar to the bash history
feature, such that if you press enter while on a search entry it will immediately
run the command. Similary, pressing `^A` will insert the text into the terminal
followed by a `^A` which will set your cursor at the beginning of the prompt if
you are in a shell.

There are several special non-text keys that will not cause aug-db to insert the
text of the top-most entry, but will instead invoke a UI command. The following
keys will control the aug-db UI as described:
 * `^C`:     closes the aug-db UI window.  
 * `^G`:     clears the current search text.  
 * `^N`:     moves down through search results.  
 * `^P`:     moves up through search results.  
 * `^]`:     moves selected result into the trash. The "trash" is simply a 
             boolean SQL field. To un-delete something you can open your sqlite
             database and set the "trash" field to 0. To delete something
             forever, you should open your sqlite DB and delete the actual row
             in the 'blobs' table.  
 * `^/`:     displays a help screen with information on these command keys.  


