#ifndef DICTIONARY_HH
#define DICTIONARY_HH


// General concept:
//
// PREREQUISITES:
// Automatic saving (every time a persistent value is changed, that value is
// synced) to a DB to avoid data loss on crashes or if the user doesn’t save.
//
// Automatic save on quit
//
// User settings: global per-user settings that are *not* stored in the project
// file, e.g. the default font to use (so multiple people can use the same project
// file with different settings). Add an option to the settings to override the
// settings with the per-user settings.
//
// Add an option to dump a Smyth project to JSON and load it from JSON so we can
// actually merge one if two people happen to make changes to the same project
// file at the same time.
//
// Add a 'Merge' button that creates a `.smyth-merge` directory, creates a new git
// repository in it, adds our file to it, commits it, makes a second branch, adds
// their file, commits it, and then merges the two; the 'Merge' button then becomes
// a 'Complete Merge' button and checks if the merge was successful; if it was, it
// loads the merged JSON file back into the project. The merge is aborted if the temporary
// directory (or the JSON file in it) is deleted for whatever reason. Check whether
// such a directory exists on startup and enter merge mode if it does.
//
// FEATURES:
//
// List view that presents entries.
//
// Separate saving logic so only rows that have changed are saved on every
// operation that changes a row.
//
// CTRL+C when an entry is selected to copy the word.
//
// Backspace/Delete to delete an entry. Always ask the user for confirmation
// before an entry is deleted. (TODO: Also support undoing deletions)
//
// Double-click an entry to *open* it. This shows the entry in a dialog
//     that contains a more detailed view of it (including e.g. all senses,
//     examples, etc.)
//
// Right-click an entry to open a menu that lets you:
//     - Delete an entry
//     - Edit an entry (this brings up another dialog)
//
// Button to add a new entry. This opens a dialog that, unlike the other
//     dialogs described above, is *NOT* modal, so people can still use
//     Smyth while adding an entry.
//
// Search bar at the top that lets you filter entries (by any column; use
//     a select menu for that); search uses NFKD on the search input; the
//     search also applies NFKD to the field being searched.
//
// Sort entries by NFKD of the headword.
//
// Export to LaTeX.

#endif //DICTIONARY_HH
