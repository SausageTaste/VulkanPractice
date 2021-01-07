import os

import local_tools.path_tools as ptt


def find_apps_dir():
    return os.path.join(ptt.find_repo_root_path(), "apps")

def isFileSourceCode(name: str) -> bool:
    SOURCE_FILE_EXT = (
        "py",
        "cpp",
        "h",
        "hpp",
    )

    for ext in SOURCE_FILE_EXT:
        if name.endswith("." + ext):
            return True
    return False

def countlines(start, lines=0, header=True, begin_start=None, recursive=False):
    if header:
        print('{:>10} |{:>10} | {:<20}'.format('ADDED', 'TOTAL', 'FILE'))
        print('{:->11}|{:->11}|{:->20}'.format('', '', ''))

    for thing in os.listdir(start):
        thing = os.path.join(start, thing)

        if not os.path.isfile(thing): continue
        if not isFileSourceCode(thing): continue

        with open(thing, 'r') as f:
            newlines = f.readlines()
            newlines = len(newlines)
            lines += newlines

            if begin_start is not None:
                reldir_of_thing = '.' + thing.replace(begin_start, '')
            else:
                reldir_of_thing = '.' + thing.replace(start, '')

            print('{:>10} |{:>10} | {:<20}'.format(newlines, lines, reldir_of_thing))

    if recursive:
        for thing in os.listdir(start):
            thing = os.path.join(start, thing)
            if os.path.isdir(thing):
                lines = countlines(thing, lines, header=False, begin_start=start)

    return lines


def main():
    apps_dir = find_apps_dir()
    print(apps_dir)
    totalLineCount = countlines(apps_dir)
    print("Total lines: {}".format(totalLineCount))
    input("\nPress a key to continue...")


if "__main__" == __name__:
    main()
