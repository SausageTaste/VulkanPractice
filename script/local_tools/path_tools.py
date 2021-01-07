import os


def find_repo_root_path():
    cur_path = "."

    for _ in range(5):
        dirs_in_fol = os.listdir(cur_path)
        if "apps" in dirs_in_fol:
            return cur_path
        else:
            cur_path = os.path.join(cur_path, "..")

    raise RuntimeError("apps folder not found")
